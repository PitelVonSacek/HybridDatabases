prace.pdf: *.tex
	( cd mereni; ./make_graph.sh )
	pdflatex -interaction=batchmode prace.tex || true
	pdflatex -interaction=batchmode prace.tex || true
	pdflatex -interaction=batchmode prace.tex || true
	${MAKE} clean

.PHONY: vlna clean aspell

vlna: *.tex
	vlna -v KkSsVvZzOoUuAIai prace.tex uvod.tex kap*.tex zaver.tex abstract_{en,cs}.tex appendix.tex
	${MAKE} clean

aspell: *.tex
	bash -c 'for file in prace.tex uvod.tex kap*.tex zaver.tex; do aspell -c "$$file"; done'
	${MAKE} clean

clean:
	rm -f *.aux *.log *.toc *.out *.te~ *.tex.bak mereni/*.tex mereni/*.pdf mereni/*.plt

