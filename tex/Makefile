LATEXC := pdflatex
GLOC := makeglossaries
BIBC := bibtex

all : proposal.pdf

%.pdf : %.tex
	$(LATEXC) $<
	$(BIBC) $(patsubst %.tex, %, $<)
	$(LATEXC) $<
	$(LATEXC) $<
	rm -f *.aux *.bbl *.blg *.log *.out *.toc
