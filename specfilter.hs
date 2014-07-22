#!/usr/bin/env runhaskell

import Text.Pandoc.JSON
import Text.Pandoc.Walk

main = toJSONFilter go
  where go :: Pandoc -> Pandoc
        go = walk exampleDivs . walk anchors

exampleDivs :: Block -> Block
exampleDivs (Div (ident, ["example"], kvs)
            [ d@(Div (_,["examplenum"],_) _),
              c1@(CodeBlock (_,["markdown"],_) _),
              c2@(CodeBlock (_,["html"],_) _)
            ]) = Div (ident, ["example"], kvs)
            [ rawtex "\\begin{minipage}[t]{\\textwidth}\n{\\scriptsize "
            , d
            , rawtex "\\vspace{-1em}}"
            , rawtex "\\begin{minipage}[t]{0.49\\textwidth}\n\\definecolor{shadecolor}{gray}{0.85}\n"
            , addBreaks c1
            , rawtex "\\end{minipage}\n\\hfill\n\\begin{minipage}[t]{0.49\\textwidth}\n\\definecolor{shadecolor}{gray}{0.95}\n"
            , addBreaks c2
            , rawtex "\\end{minipage}\n\\end{minipage}"
            ]
  where rawtex = RawBlock (Format "latex")
        addBreaks (CodeBlock attrs code) = CodeBlock attrs $ addBreaks' code
        addBreaks' code =
          if length code > 49
             then take 49 code ++ ('\n':addBreaks' (drop 49 code))
             else code
exampleDivs x = x

anchors :: Inline -> Inline
anchors (RawInline (Format "html") ('<':'a':' ':'i':'d':'=':'"':xs)) =
  RawInline (Format "latex") ("\\hyperdef{}{" ++ lab ++ "}{\\label{" ++ lab ++ "}}")
  where lab = takeWhile (/='"') xs
anchors x = x
