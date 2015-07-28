#!/usr/bin/env runhaskell

import Text.Pandoc.JSON
import Text.Pandoc.Generic

main = toJSONFilter go
  where go :: Pandoc -> Pandoc
        go = bottomUp exampleDivs . bottomUp (concatMap anchors)

exampleDivs :: Block -> Block
exampleDivs (Div (ident, ["example"], kvs)
            [ d@(Div (_,["examplenum"],_) _),
              c1@(CodeBlock (_,["markdown"],_) _),
              c2@(CodeBlock (_,["html"],_) _)
            ]) = Div (ident, ["example"], kvs)
            [ rawtex "\\begin{minipage}[t]{\\textwidth}\n{\\scriptsize "
            , d
            , rawtex "}\\vspace{-0.4em}\n"
            , rawtex "\\begin{minipage}[t]{0.49\\textwidth}\n\\definecolor{shadecolor}{gray}{0.85}\n\\begin{snugshade}\\small\n"
            , addBreaks c1
            , rawtex "\\end{snugshade}\n\\end{minipage}\n\\hfill\n\\begin{minipage}[t]{0.49\\textwidth}\n\\definecolor{shadecolor}{gray}{0.95}\n\\begin{snugshade}\\small\n"
            , addBreaks c2
            , rawtex "\\end{snugshade}\n\\end{minipage}\n\\end{minipage}"
            ]
  where rawtex = RawBlock (Format "latex")
        addBreaks (CodeBlock attrs code) = CodeBlock attrs $ addBreaks' code
        addBreaks' code =
          if length code > 49
             then take 49 code ++ ('\n':addBreaks' (drop 49 code))
             else code
exampleDivs x = x

anchors :: Inline -> [Inline]
anchors (Link text ('@':lab,_)) =
  [RawInline (Format "latex") ("\\hyperdef{}{" ++ lab ++ "}{\\label{" ++ lab ++ "}}"), Strong text]
anchors (Span ("",["number"],[]) xs) = [] -- remove sect numbers
anchors x = [x]
