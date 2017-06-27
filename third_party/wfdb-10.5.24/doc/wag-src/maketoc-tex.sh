#! /bin/sh
# file: maketoc-tex.sh		G. Moody	29 October 2002
#				Last revised:	 13 March 2014
# Generate the table of contents and appendices for the WFDB Applications Guide

countpages()
{
    pdftk $1 dump_data | grep NumberOfPages | cut -c 16-
}

prep()
{
    ( for F in *.1 *.3 *.5; do grep "\\\\-" $F | head -1; done ) >namelines.out
    ./getpagenos <wag2.ps >pagenos.out
    pr -m -s -T namelines.out pagenos.out | \
     sed "s+\\\\fB+\\\\textbf{ +" | \
     sed "s+\\\\fR+}+" >toc.out
    rm -f namelines.out pagenos.out
}

appendices()
{
    P=`countpages wag2.pdf`
    case $P in
      *[13579]) pdftk wag2.pdf blankpage cat output wag2b.pdf
	    mv wag2b.pdf wag2.pdf
	    P=`expr $P + 2`;;
      *) P=`expr $P + 1`;;
    esac
    sed s/FIRSTPAGE/$P/ <install0.tex >install.tex
    make wag3.pdf
    N=`countpages wag3.pdf`
    Q=`expr $P + $N`
    case $Q in
      *[02468]) pdftk wag3.pdf blankpage cat output wag3b.pdf;
	    mv wag3b.pdf wag3.pdf
	    Q=`expr $Q + 1`;;
    esac
    sed s/FIRSTPAGE/$Q/ <eval0.tex >eval.tex
    make wag4.pdf
}

prep >toc-log.$$ 2>&1

cat <<EOF

\contentsline{chapter}{{\large Introduction}}{v}
\contentsline{chapter}{{\large Frequently Asked Questions}}{vii}
\contentsline{chapter}{{\large Section 1: Applications}}{}

EOF

N1=`ls *.1 | wc -l | tr -d " "`
head -$N1 <toc.out | ./maketoclines

appendices >>toc-log.$$ 2>&1

cat <<EOF

\contentsline{chapter}{{\large Section 3: WFDB libraries}}{}

EOF

NT=`wc -l <toc.out | tr -d " "`
NR=`expr $NT - $N1`
N3=`ls *.3 | wc -l | tr -d " "`
tail -$NR <toc.out | head -$N3 | ./maketoclines

cat <<EOF

\contentsline{chapter}{{\large Section 5: WFDB file formats}}{}

EOF

NR=`expr $NT - $N1 - $N3`
N5=`ls *.5 | wc -l | tr -d " "`
tail -$NR <toc.out | head -$N5 | ./maketoclines

cat <<EOF

\contentsline{chapter}{{\large Appendices}}{}

\contentsline {section}{Installing the WFDB Software Package}{$P}
\contentsline {section}{Evaluating ECG Analyzers}{$Q}
EOF

rm -f getpagenos maketoclines toc.out toc-log.$$
