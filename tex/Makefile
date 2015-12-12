LATEXC := pdflatex
GLOC := makeglossaries
BIBC := bibtex

all : final_report.pdf proposal.pdf

%.pdf : %.tex
	$(LATEXC) $<
	$(BIBC) $(patsubst %.tex, %, $<)
	$(LATEXC) $<
	$(LATEXC) $<
	rm -f *.aux *.bbl *.blg *.log *.out *.toc

%.pdf : %.svg
	convert $< $@

final_report.pdf : logo.pdf references.bib
