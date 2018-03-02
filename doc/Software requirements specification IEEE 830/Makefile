TEX = pdflatex --interaction batchmode
BIB = bibtex

TARGET = srs
REF = references.bib

RM = rm -f

all: $(TARGET).pdf

$(TARGET).pdf: $(TARGET).tex $(REF)
	@echo "TEX $(TARGET)"
	@$(TEX) $(TARGET) > /dev/null
	@echo "TEX $(TARGET)"
	@$(TEX) $(TARGET) > /dev/null
	@echo "BIB $(TARGET)"
	@$(BIB) $(TARGET) > /dev/null
	@echo "TEX $(TARGET)"
	@$(TEX) $(TARGET) > /dev/null
	@echo "TEX $(TARGET)"
	@$(TEX) $(TARGET) > /dev/null

.PHONY: clean

clean:
	@echo "RM $(TARGET).aux"
	@$(RM) $(TARGET).aux
	@echo "RM $(TARGET).bbl"
	@$(RM) $(TARGET).bbl
	@echo "RM $(TARGET).blg"
	@$(RM) $(TARGET).blg
	@echo "RM $(TARGET).log"
	@$(RM) $(TARGET).log
	@echo "RM $(TARGET).out"
	@$(RM) $(TARGET).out
	@echo "RM $(TARGET).pdf"
	@$(RM) $(TARGET).pdf
	@echo "RM $(TARGET).toc"
	@$(RM) $(TARGET).toc
