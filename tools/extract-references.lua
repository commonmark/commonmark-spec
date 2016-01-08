-- append list of reference definitions from links of the
-- form [foo](@foo-id)
-- modify anchor links

local trim = function(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end

return function(doc, meta, format)
  local cur, entering, node_type
  local refs = {}
  for cur, entering, node_type in walk(doc) do
      if node_type == NODE_LINK then
          local url = node_get_url(cur)
          if url:match("^@") then
              local ident = url:sub(2)
              local children = node_first_child(cur)
              refs[#refs + 1] = '[' ..
                   trim(render_commonmark(children, OPT_DEFAULT, 0)) ..
                   ']: #' .. ident
              node_set_url(cur, '#' .. ident)
          end
      end
  end

  local refs_node = node_new(NODE_CUSTOM_BLOCK)
  node_set_on_enter(refs_node, table.concat(refs, '\n'))
  node_insert_after(node_last_child(doc), refs_node)
  -- now append to end
end

