LATEXC := pdflatex
GLOC := makeglossaries
BIBC := bibtex

all : final_report.pdf proposal.pdf

clean :
	rm -f *.aux *.bbl *.blg *.glo *.ist *.log *.out final_report.pdf proposal.pdf *.toc

%.pdf : %.tex
	$(LATEXC) $<
	$(BIBC) $(patsubst %.tex, %, $<)
	$(LATEXC) $<
	$(LATEXC) $<
	rm -f *.aux *.bbl *.blg *.glo *.ist *.log *.out *.toc

%.pdf : %.svg
	convert $< $@

final_report.pdf : \
	glossary.tex \
	logo.pdf \
	references.bib
