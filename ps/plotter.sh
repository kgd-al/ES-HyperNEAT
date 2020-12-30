#!/bin/bash

cd $(dirname $0)
mkdir -pv tmp/
cd tmp

functions="
abs;abs(x)
gaus;e^(-(x*x)/4)
id;x
sigm;1/(1+e^-x)
sin;sin(deg(x))
step;x<0?0:1"

# functions="step;x<0?0:1"

for line in $functions
do
    IFS=';' read name func <<< "$line"
    echo "$name: $func"
    sed 's|\\def\\func{.*}|\\def\\func{'"$func"'}|' ../template.tex > $name.tex
    latex $name.tex >/dev/null && dvips -F* -s* -q $name.dvi -o $name.eps && eps2eps -f $name.eps ../$name.eps
    convert -density 100 ../$name.eps ../$name.png
#     mv $name.eps ../
    echo
done

cd ..
rm -r tmp
ls -lh *ps *.png
          
