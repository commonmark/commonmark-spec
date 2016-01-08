local cmark = require('cmark')

-- extract list of reference definitions from links of the
-- form [foo](@foo-id)

local trim = function(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end

local inp = io.read("*a")

-- strip off yaml metadata so it doesn't interfere with parsing:
local _, endyaml = string.find(inp, "[\n\r]+%.%.%.[\n\r]")
inp = inp:sub(endyaml)

local doc = cmark.parse_string(inp, cmark.OPT_DEFAULT)
io.write('\n')

local cur, entering, node_type
for cur, entering, node_type in cmark.walk(doc) do
    if node_type == cmark.NODE_LINK then
        local url = cmark.node_get_url(cur)
        if url:match("^@") then
            local ident = url:sub(2)
            local children = cmark.node_first_child(cur)
            io.write('[' ..
                 trim(cmark.render_commonmark(children, cmark.OPT_DEFAULT, 0)) ..
                 ']: #' .. ident .. '\n')
            cmark.node_set_url(cur, '#' .. ident)
        end
    end
end

