#!/usr/bin/env ruby
require 'ffi'
require 'stringio'
require 'cgi'
require 'set'
require 'uri'

module CMark
  extend FFI::Library
  ffi_lib ['libcmark', 'cmark']
  typedef :pointer, :node
  enum :node_type, [:document, :blockquote, :list, :list_item,
                    :code_block, :html, :paragraph,
                    :header, :hrule, :reference_def,
                    :str, :softbreak, :linebreak, :code, :inline_html,
                    :emph, :strong, :link, :image]
  enum :list_type, [:no_list, :bullet_list, :ordered_list]

  attach_function :cmark_free_nodes, [:node], :void
  attach_function :cmark_node_unlink, [:node], :void

  attach_function :cmark_markdown_to_html, [:string, :int], :string
  attach_function :cmark_parse_document, [:string, :int], :node
  attach_function :cmark_node_first_child, [:node], :node
  attach_function :cmark_node_parent, [:node], :node
  attach_function :cmark_node_next, [:node], :node
  attach_function :cmark_node_previous, [:node], :node
  attach_function :cmark_node_get_type, [:node], :node_type
  attach_function :cmark_node_get_string_content, [:node], :string
  attach_function :cmark_node_get_url, [:node], :string
  attach_function :cmark_node_get_title, [:node], :string
  attach_function :cmark_node_get_header_level, [:node], :int
  attach_function :cmark_node_get_list_type, [:node], :list_type
  attach_function :cmark_node_get_list_start, [:node], :int
  attach_function :cmark_node_get_list_tight, [:node], :bool
end

class Node
  attr_accessor :type, :children, :parent, :string_content, :header_level,
                :list_type, :list_start, :list_tight, :url, :title
  def initialize(pointer)
    if pointer.null?
      return nil
    end
    @pointer = pointer
    @type = CMark::cmark_node_get_type(pointer)
    @children = []
    @parent = nil
    first_child = CMark::cmark_node_first_child(pointer)
    b = first_child
    while !b.null?
      child = Node.new(b)
      child.parent = self
      @children << child
      b = CMark::cmark_node_next(b)
    end
    @string_content = CMark::cmark_node_get_string_content(pointer)
    if @type == :header
      @header_level = CMark::cmark_node_get_header_level(pointer)
    end
    if @type == :list
      @list_type = CMark::cmark_node_get_list_type(pointer)
      @list_start = CMark::cmark_node_get_list_start(pointer)
      @list_tight = CMark::cmark_node_get_list_tight(pointer)
    end
    if @type == :link || @type == :image
      @url = CMark::cmark_node_get_url(pointer)
      if !@url then @url = "" end
      @title = CMark::cmark_node_get_title(pointer)
      if !@title then @title = "" end
    end
    if @type == :document
      self.free
    end
  end

  # An iterator that "walks the tree," returning each node
  def walk(&blk)
    yield self
    self.children.each do |child|
      child.walk(&blk)
    end
  end

  # Walk the tree and transform it.  blk should take one argument,
  # a node.  If its value is a node, that node replaces the node being
  # visited. If its value is an array of nodes, those nodes are spliced
  # in place of the node being visited (so, to delete a node, use an
  # empty array).  Otherwise the node is left as it is.
  def transform(&blk)
    self.walk do |node|
      skip = false
      res = blk.call(node)
      if res.kind_of?(Array)
        splice = res
      elsif res.kind_of?(Node)
        splice = [res]
      else
        skip = true
      end
      unless skip
        parent = node.parent
        if parent
          siblings = node.parent.children
          index = siblings.index(node)
          siblings.replace(siblings.slice(0,index) + splice +
                           siblings.slice(index + 1, siblings.length))
        else # at the document root, just skip
        end
      end
    end
  end

  def self.parse_string(s)
    Node.new(CMark::cmark_parse_document(s, s.bytesize))
  end

  def self.parse_file(f)
    s = f.read()
    self.parse_string(s)
  end

  protected
  def free
      CMark::cmark_free_nodes(@pointer)
  end
end

class Renderer
  attr_accessor :in_tight, :warnings, :in_plain
  def initialize(stream = nil)
    if stream
      @stream = stream
      @stringwriter = false
    else
      @stringwriter = true
      @stream = StringIO.new
    end
    @need_blocksep = false
    @warnings = Set.new []
    @in_tight = false
    @in_plain = false
  end

  def out(*args)
    args.each do |arg|
      if arg.kind_of?(String)
        @stream.write(arg)
      elsif arg.kind_of?(Node)
        self.render(arg)
      elsif arg.kind_of?(Array)
        arg.each { |x| self.out(x) }
      else
        @stream.write(arg)
      end
    end
  end

  def render(node)
    @node = node
    if node.type == :document
      self.document(node)
      self.out("\n")
      if @stringwriter
        return @stream.string
      end
    elsif self.in_plain && node.type != :str && node.type != :softbreak
      # pass through looking for str, softbreak
      node.children.each do |child|
        render(child)
      end
    else
      begin
        self.send(node.type, node)
      rescue NoMethodError => e
        @warnings.add("WARNING:  " + node.type.to_s + " not implemented.")
        raise e
      end
    end
  end

  def document(node)
    self.out(node.children)
  end

  def code_block(node)
    self.code_block(node)
  end

  def reference_def(node)
  end

  def blocksep
    self.out("\n")
  end

  def containersep
    if !self.in_tight
      self.out("\n")
    end
  end

  def block(&blk)
    if @need_blocksep
      self.blocksep
    end
    blk.call
    @need_blocksep = true
  end

  def container(starter, ender, &blk)
    self.out(starter)
    self.containersep
    @need_blocksep = false
    blk.call
    self.containersep
    self.out(ender)
  end

  def plain(&blk)
    old_in_plain = @in_plain
    @in_plain = true
    blk.call
    @in_plain = old_in_plain
  end
end

class HtmlRenderer < Renderer
  def header(node)
    block do
      self.out("<h", node.header_level, ">", node.children,
             "</h", node.header_level, ">")
    end
  end

  def paragraph(node)
    block do
      if self.in_tight
        self.out(node.children)
      else
        self.out("<p>", node.children, "</p>")
      end
    end
  end

  def list(node)
    old_in_tight = self.in_tight
    self.in_tight = node.list_tight
    block do
      if node.list_type == :bullet_list
        container("<ul>", "</ul>") do
          self.out(node.children)
        end
      else
        start = node.list_start == 1 ? '' :
                (' start="' + node.list_start.to_s + '"')
        container(start, "</ol>") do
          self.out(node.children)
        end
      end
    end
    self.in_tight = old_in_tight
  end

  def list_item(node)
    block do
      container("<li>", "</li>") do
        self.out(node.children)
      end
    end
  end

  def blockquote(node)
    block do
      container("<blockquote>", "</blockquote>") do
        self.out(node.children)
      end
    end
  end

  def hrule(node)
    block do
      self.out("<hr />")
    end
  end

  def code_block(node)
    block do
      self.out("<pre><code>")
      self.out(CGI.escapeHTML(node.string_content))
      self.out("</code></pre>")
    end
  end

  def html(node)
    block do
      self.out(node.string_content)
    end
  end

  def inline_html(node)
    self.out(node.string_content)
  end

  def emph(node)
    self.out("<em>", node.children, "</em>")
  end

  def strong(node)
    self.out("<strong>", node.children, "</strong>")
  end

  def link(node)
    self.out('<a href="', URI.escape(node.url), '"')
    if node.title && node.title.length > 0
      self.out(' title="', CGI.escapeHTML(node.title), '"')
    end
    self.out('>', node.children, '</a>')
  end

  def image(node)
    self.out('<img src="', URI.escape(node.url), '"')
    if node.title && node.title.length > 0
      self.out(' title="', CGI.escapeHTML(node.title), '"')
    end
    plain do
      self.out(' alt="', node.children, '" />')
    end
  end

  def str(node)
    self.out(CGI.escapeHTML(node.string_content))
  end

  def code(node)
    self.out("<code>")
    self.out(CGI.escapeHTML(node.string_content))
    self.out("</code>")
  end

  def linebreak(node)
    self.out("<br/>")
    self.softbreak(node)
  end

  def softbreak(node)
    self.out("\n")
  end
end

doc = Node.parse_file(ARGF)

# Walk tree and print URLs for links
doc.walk do |node|
  if node.type == :link
    printf("URL = %s\n", node.url)
  end
end

# Capitalize strings in headers
doc.walk do |node|
  if node.type == :header
    node.walk do |subnode|
      if subnode.type == :str
        subnode.string_content = subnode.string_content.upcase
      end
    end
  end
end

# Walk tree and transform links to regular text
doc.transform do |node|
  if node.type == :link
    node.children
  end
end

renderer = HtmlRenderer.new(STDOUT)
renderer.render(doc)
renderer.warnings.each do |w|
  STDERR.write(w)
  STDERR.write("\n")
end
# def markdown_to_html(s)
#   len = s.bytes.length
#   CMark::cmark_markdown_to_html(s, len)
# end
# print markdown_to_html(STDIN.read())
