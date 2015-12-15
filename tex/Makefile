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

%.pdf : %.tif
	convert $< $@

%.pdf : %.jpg
	convert $< $@

%.svg : %.dot
	dot -Tsvg $< > $@

final_report.pdf : \
	BodyCamBoard_brd.pdf \
	BodyCamBoard_sch.pdf \
	IOBoard_brd.pdf \
	IOBoard_sch.pdf \
	budget.tex \
	exploded_enclosure.pdf \
	glossary.tex \
	logo.pdf \
	references.bib \
	subsys_video.pdf \
	sys_desc.pdf \
	trigger_flow.png \
	uvc.h
