all: srs.pdf

srs.pdf: srs.tex references.bib
	pdflatex srs
	pdflatex srs
	bibtex srs
	pdflatex srs
	pdflatex srs

.PHONY: clean distclean

clean:
	rm -f srs.aux
	rm -f srs.bbl
	rm -f srs.blg
	rm -f srs.log
	rm -f srs.out
	rm -f srs.toc

distclean: clean
	rm -f srs.pdf
