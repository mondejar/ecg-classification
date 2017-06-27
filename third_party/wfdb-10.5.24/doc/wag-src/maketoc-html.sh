#! /bin/sh
# file: maketoc-html.sh		G. Moody	29 October 2002
#
# Generate the HTML table of contents for the WFDB Applications Guide

necho()
{
    if [ -s echo.$$ ]
    then echo $* "\\c"
    else echo -n $*
    fi
}

tocentry()
{
    case $1 in
      *.1) b=`basename $1 .1`; s=1 ;;
      *.3) b=`basename $1 .3`; s=3 ;;
      *.5) b=`basename $1 .5`; s=5 ;;
      *.7) b=`basename $1 .7`; s=7 ;;
      *) b=$1; s=x ;;
    esac
    case $b in
      ???????*) b=`echo $b | cut -c 1-6` ;;
    esac
    necho '<li> <a href="'
    necho "$b-$s.htm"
    necho '">'
    grep "\\\\-" $1 | head -1 | sed "s+ \\\\- +</a>: +" | \
      sed "s+\\\\fB+<b>+" | sed "s+\\\\fR+</b>+"
}

echo -n >echo.$$

cat wag.ht0
cat <<EOF
<h3>Applications</h3>
<ul> 
EOF

for F in *.1
do
    tocentry $F
done

cat <<EOF
</ul>
<h3>WFDB libraries</h3>
<ul> 
EOF

for F in *.3
do
    tocentry $F
done

cat <<EOF
</ul>
<h3>WFDB file formats</h3>
<ul> 
EOF

for F in *.5
do
    tocentry $F
done

cat <<EOF
</ul>
<h3>Miscellaneous</h3>
<ul> 
EOF

for F in *.7
do
    tocentry $F
done

cat <<EOF
</ul>
EOF

cat wag.ht1
rm echo.$$
