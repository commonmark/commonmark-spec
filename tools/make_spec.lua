local lcmark = require('lcmark')
local cmark = require('cmark')

local trim = function(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end

local warn = function(s)
  io.stderr:write('WARNING: ' .. s .. '\n')
end

local to_identifier = function(s)
  return trim(s):lower():gsub('[^%w]+', ' '):gsub('[%s]+', '-')
end

local render_number = function(tbl)
  local res = ''
  for _,x in ipairs(tbl) do
    res = res .. tostring(x) .. '.'
  end
  return res
end

local extract_references = function(doc)
  local cur, entering, node_type
  local refs = {}
  for cur, entering, node_type in cmark.walk(doc) do
    if not entering and
        ((node_type == cmark.NODE_LINK and cmark.node_get_url(cur) == '@') or
          node_type == cmark.NODE_HEADING) then
      local children = cmark.node_first_child(cur)
      local label = trim(cmark.render_commonmark(children, OPT_DEFAULT, 0))
      local ident = to_identifier(label)
      if refs[label] then
        warn("duplicate reference " .. label)
      end
      refs[label] = ident
    end
  end
  return refs
end

local create_anchors = function(doc, meta, to)
  local cur, entering, node_type
  local toc = {}
  local number = {0}
  for cur, entering, node_type in cmark.walk(doc) do
    if not entering and
        ((node_type == cmark.NODE_LINK and cmark.node_get_url(cur) == '@') or
          node_type == cmark.NODE_HEADING) then
      local anchor = cmark.node_new(
        node_type == cmark.NODE_LINK and cmark.NODE_CUSTOM_INLINE or
        cmark.NODE_CUSTOM_BLOCK)
      local children = cmark.node_first_child(cur)
      local label = trim(cmark.render_commonmark(children, OPT_DEFAULT, 0))
      local ident = to_identifier(label)
      if node_type == cmark.NODE_LINK then
        cmark.node_set_on_enter(anchor, '<a id="' .. ident .. '"' ..
               ' href="#' .. ident .. '" class="definition">')
        cmark.node_set_on_exit(anchor, "</a>")
      else -- NODE_HEADING
        local level = cmark.node_get_heading_level(cur)
        local last_level = #toc == 0 and 1 or toc[#toc].level
        if level > last_level then -- subhead
          number[level] = 1
        else
          while last_level > level do
            number[last_level] = nil
            last_level = last_level - 1
          end
          number[level] = number[level] + 1
        end
        table.insert(toc, { label = label, ident = indent, level = level })
        cmark.node_set_on_enter(anchor, '<h' ..
           tostring(level) .. ' id="' .. ident ..  '">' ..
           '<span class="number">' .. render_number(number) .. '</span> ')
        cmark.node_set_on_exit(anchor, '</h' .. tostring(level) .. '>')
      end
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

local to_ref = function(ref)
  return '[' .. ref.label .. ']: #' .. ref.indent .. '\n'
end

format = 'html'

io.input("spec.txt")
local inp = io.read("*a")
local doc1 = cmark.parse_string(inp, cmark.OPT_DEFAULT)
local refs = extract_references(doc1)
local refblock = '\n'
for lab,ident in pairs(refs) do
  refblock = refblock .. '[' .. lab .. ']: #' .. ident .. '\n'
  -- refblock = refblock .. '[' .. lab .. 's]: #' .. ident .. '\n'
end
-- append references and parse again
local html, meta, msg  = lcmark.convert(inp .. refblock, format,
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

