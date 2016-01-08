-- extract list of reference definitions from links of the
-- form [foo](@foo-id)

local trim = function(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end

return function(doc, meta, format)
    local refs = {}
    local cur, entering, node_type
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
    -- Now insert the list of references at the end:
    local refsblock = node_new(NODE_CUSTOM_BLOCK)
    node_set_on_enter(refsblock, table.concat(refs, '\n'))
    local lastblock = node_last_child(doc)
    node_insert_after(lastblock, refsblock)
end

