require 'ffi'
require 'stringio'
require 'cgi'
require 'set'

module CMark
  extend FFI::Library
  ffi_lib ['libcmark', 'cmark']
  typedef :pointer, :node
  enum :node_type, [:document, :blockquote, :list, :list_item,
                    :fenced_code, :indented_code, :html, :paragraph,
                    :atx_header, :setext_header, :hrule, :reference_def,
                    :str, :softbreak, :linebreak, :code, :inline_html,
                    :emph, :strong, :link, :image]

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
  attach_function :cmark_node_get_header_level, [:node], :int
end

class Node
  attr_accessor :type, :children, :string_content, :header_level
  def initialize(pointer)
    if pointer.null?
      return nil
    end
    @pointer = pointer
    @type = CMark::cmark_node_get_type(pointer)
    @children = []
    first_child = CMark::cmark_node_first_child(pointer)
    b = first_child
    while !b.null?
      @children << Node.new(b)
      b = CMark::cmark_node_next(b)
    end
    @string_content = CMark::cmark_node_get_string_content(pointer)
    @header_level = CMark::cmark_node_get_header_level(pointer)
    if @type == :document
      self.free
    end
  end

  def self.parse_string(s)
    Node.new(CMark::cmark_parse_document(s, s.bytesize))
  end

  def self.parse_file(f)
    s = f.read()
    self.parse_string(s)
  end

  def free
      CMark::cmark_free_nodes(@pointer)
  end
end

class Renderer
  attr_reader :warnings
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
  end

  def outf(format, *args)
    @stream.printf(format, *args)
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
    else
      begin
        self.send(node.type, node)
      rescue NoMethodError
        @warnings.add("WARNING:  " + node.type.to_s + " not implemented.")
        self.out(node.children)
      end
    end
  end

  def document(node)
    self.out(node.children)
  end

  def indented_code(node)
    self.code_block(node)
  end

  def fenced_code(node)
    self.code_block(node)
  end

  def setext_header(node)
    self.header(node)
  end

  def atx_header(node)
    self.header(node)
  end

  def reference_def(node)
  end

  def blocksep
    self.out("\n\n")
  end

  def asblock(&blk)
    if @need_blocksep
      self.blocksep
    end
    blk.call
    @need_blocksep = true
  end
end

class HtmlRenderer < Renderer
  def header(node)
    asblock do
      self.out("<h", node.header_level, ">", node.children,
             "</h", node.header_level, ">")
    end
  end

  def paragraph(node)
    asblock do
      self.out("<p>", node.children, "</p>")
    end
  end

  def hrule(node)
    asblock do
      self.out("<hr />")
    end
  end

  def code_block(node)
    asblock do
      self.out("<pre><code>")
      self.out(CGI.escapeHTML(node.string_content))
      self.out("</code></pre>")
    end
  end

  def emph(node)
    self.out("<em>", node.children, "</em>")
  end

  def strong(node)
    self.out("<strong>", node.children, "</strong>")
  end

  def str(node)
    self.out(CGI.escapeHTML(node.string_content))
  end

  def code(node)
    self.out("<code>")
    self.out(CGI.escapeHTML(node.string_content))
    self.out("</code>")
  end

  def softbreak(node)
    self.out("\n")
  end
end

doc = Node.parse_file(STDIN)
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
