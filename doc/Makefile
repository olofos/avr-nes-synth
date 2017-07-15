SOURCES=block-diagram.tex
PDFS=$(SOURCES:.tex=.pdf)
IMAGES=$(SOURCES:.tex=.png)

.PHONY: all clean

all: $(IMAGES)

%.png: %.pdf
	convert -density 300 -quality 90 -flatten $< $@

%.pdf: %.tex
	pdflatex  -file-line-error   -interaction=nonstopmode "\input" $<

clean:
	rm -f $(IMAGES) $(PDFS) *.log *.aux
