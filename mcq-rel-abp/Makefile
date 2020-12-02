OUTDIR=public

MSC=$(wildcard *.msc)
PNG=$(MSC:.msc=.png)

all: $(PNG)

%.png: %.msc
	mscgen -T png -o $(OUTDIR)/$@ $<
