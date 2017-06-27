#! /bin/sh
# file: fixag.sh	G. Moody	10 April 1997
#			Last revised:    7 April 2003
#
# Post-process WFDB Applications Guide HTML files

URLPREFIX=http://www.physionet.org/physiotools/wag/

LONGDATE=$1
shift

for i in $*
do
  F=`basename $i .html`
  G=`echo $F | sed 's/\./-/g'`
  echo processing $F ...
  sed 's/\.html/\.htm/g' <$F.html |
  sed 's/evfootnode\.htm/evfoot\.htm/g' |
  sed 's/<BODY >/<BODY BGCOLOR="#FFFFFF">/' |
  sed 's/\.\(.\)\.htm/-\1.htm/g' >$G.htm
done
mv -f evfootnode.htm evfoot.htm

for i in *.htm
do
  echo fixing links in $i ...
  cp $i .fix.$$
  sed -f fixag.sed <.fix.$$ | sed "s/LONGDATE/$LONGDATE/" |
    sed "s+PAGENAME+$URLPREFIX$i+" >$i
done
rm .fix.$$

PREVT=
PREVU=
THIST=FAQ
THISU="faq.htm"
NEXTT=
NEXTU=

for i in *-1.htm *-3.htm *-5.htm *-7.htm
do
  NEXTT=`grep "<TITLE>" $i | sed "s+<TITLE>++" | sed "s+</TITLE>++"`
  if [ "x$NEXTT" = "x" ]
  then
    NEXTT=`grep "<title>" $i | sed "s+<title>++" | sed "s+</title>++"`
  fi
  NEXTU=$i
  if [ "$THISU" = "faq.htm" ]
  then
    sed "s+NEXTPAGE+<a href=$NEXTU>$NEXTT</a>+" <$THISU >tmp.$$
  else
    sed "s+NEXTPAGE+<a href=$NEXTU>$NEXTT</a>+" <$THISU |
     sed "s+>record+><a href=\"intro.htm#record\">record</a>+g" |
     sed "s+>annotator+><a href=\"intro.htm#annotator\">annotator</a>+g" |
     sed "s+[a-zA-Z]*-annotator+<a href=\"intro.htm#annotator\">&</a>+g" |
     sed "s+>\\(ann[1-3]\\)\\([ <]\\)+><a href=\"intro.htm#annotator\">\\1</a>\\2+g" |
     sed "s+>time+><a href=\"intro.htm#time\">time</a>+g" |
     sed "s+>signal-list+><a href=\"intro.htm#signal-list\">signal-list</a>+g" |
     sed "s+>signal</i+><a href=\"intro.htm#signal\">signal</a></i+g" |
     sed "s+PREVPAGE+<a href=$PREVU>$PREVT</a>+" >tmp.$$
  fi
  mv tmp.$$ $THISU
  PREVT=$THIST
  PREVU=$THISU
  THIST=$NEXTT
  THISU=$NEXTU
  echo adding previous/next links in $THISU ...
done

sed "s+NEXTPAGE+<a href=install.htm>Installing the WFDB Software Package</a>+" <$THISU | \
  sed "s+PREVPAGE+<a href=$PREVU>$PREVT</a>+" >tmp.$$
mv tmp.$$ $THISU

echo adding previous links in install.htm ...
sed "s+<IMG WIDTH=\"63\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"previous\" SRC=\"prev_gr.png\">+<a href=$THISU><IMG WIDTH=\"63\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"previous\" SRC=\"previous.png\"></a>+" <install.htm |
 sed "s+WFDB Applications Guide</A>+WFDB Applications Guide</A> <b>Previous:</b> <a href=$THISU>$THIST</a>+" >tmp.$$
mv tmp.$$ install.htm

echo adding next links in innode5.htm ...
sed "s+<IMG WIDTH=\"37\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"next\" SRC=\"next_gr.png\">+<a href=\"eval.htm\"><IMG WIDTH=\"37\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"next\" SRC=\"next.png\"></a>+" <innode5.htm |
 sed "s+<B>Up:</B> <A NAME+<B>Next:</B> <a href=\"eval.htm\">Evaluating ECG Analyzers</a> <B>Up:</B> <A NAME+" >tmp.$$
mv tmp.$$ innode5.htm

echo adding previous links in eval.htm ...
sed "s+<IMG WIDTH=\"63\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"previous\" SRC=\"prev_gr.png\">+<a href=\"innode5.htm\"><IMG WIDTH=\"63\" HEIGHT=\"24\" ALIGN=\"BOTTOM\" BORDER=\"0\" ALT=\"previous\" SRC=\"previous.png\"></a>+" <eval.htm |
 sed "s+WFDB Applications Guide</A>+WFDB Applications Guide</A> <b>Previous:</b> <a href=\"innode5.htm\">Other systems</a>+" >tmp.$$
mv tmp.$$ eval.htm
