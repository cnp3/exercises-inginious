OUTDIR=public

TEXFILES=$(wildcard *.tex)
PNG=$(TEXFILES:.tex=.png)

all: $(PNG)

%.png: %.tex
	latex -shell-escape $<
	cp $@ public
