local lcmark = require('lcmark')
local cmark = require('cmark')

local trim = function(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end

local extract_references = function(doc)
  local cur, entering, node_type
  local refs = {}
  for cur, entering, node_type in cmark.walk(doc) do
    if not entering and node_type == cmark.NODE_LINK then
      local url = cmark.node_get_url(cur)
      if url:match("^@") then
        local ident = url:sub(2)
        local children = cmark.node_first_child(cur)
        refs[#refs + 1] = '[' ..
          trim(cmark.render_commonmark(children, OPT_DEFAULT, 0)) ..
          ']: #' .. ident
      end
    end
  end
  return table.concat(refs, '\n')
end

local create_anchors = function(doc, meta, to)
  local cur, entering, node_type
  for cur, entering, node_type in cmark.walk(doc) do
    if not entering and node_type == cmark.NODE_LINK
    and cmark.node_get_url(cur):match("^@") then
      local anchor = cmark.node_new(NODE_CUSTOM_INLINE)
      cmark.node_set_on_enter(anchor, "{{")
      cmark.node_set_on_exit(anchor, "}}")
      local children = cmark.node_first_child(cur)
      while children do
        node_append_child(anchor, children)
        children = cmark.node_next(children)
      end
      cmark.node_insert_before(cur, anchor)
      cmark.node_unlink(cur)
    end
  end
end

local make_examples = function(doc, meta, to)

end

format = 'html'

io.input("spec.txt")
local inp = io.read("*a")
local doc1 = cmark.parse_string(inp, cmark.OPT_DEFAULT)
local refs = extract_references(doc1)
-- append references and parse again
local html, meta, msg  = lcmark.convert(inp .. refs, format,
                             { smart = true,
                               yaml_metadata = true,
                               filters = { create_anchors,
                                           make_examples
                                         }
                             })

if html then
  local f = io.open("tools/template.html", 'r')
  if not f then
    io.write("Could not find template!")
    os.exit(1)
  end
  local template = f:read("*a")
  meta.body = html
  local rendered, msg = lcmark.render_template(template, meta)
  if not rendered then
    io.write(msg)
    os.exit(1)
  end
  io.write(rendered)
  os.exit(0)
else
  io.write(msg)
  os.exit(1)
end

