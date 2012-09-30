#!/bin/bash

avg() {
  local sum=0 count=0 i=0

  for i in "$@"; do
    : $(( sum += $i )) $(( count++ ))
  done

  echo $(( sum / count ))
}


tabulka() {
  local runs=$1 input="$2"

  {
    echo '\begin{tabular}{| r | *{'$runs'}{r} | r |} \hline'
    echo -n "Počet vláken & "
    for ((i=1; i<=runs; i++)); do echo -n "Běh $i & "; done
    echo 'Průměr \\ \hline'
    
    while read l; do
      sed -re 's/,/ \& /g' <<<"$l"
      echo -n " & "
      avg $(sed -re 's/^[^,]*//;s/,/ /g' <<<"$l")
      echo ' \\'
    done <"$input.csv"
 
    echo '\hline \end{tabular}'
  } >"$input.tex"
}


############ READ ###############

while read l; do
  echo `sed -re 's/,.*/ /' <<<"$l"``avg $(sed -re 's/^[^,]*//;s/,/ /g' <<<"$l")`
done <"read.csv" >"read.plt"

cat <<EOF | gnuplot
  set terminal pdf
  set output "read.pdf"
  set title "Čtení"
  set ylabel "Čas [ms]"
  set xlabel "Počet vláken"
  set yrange [0:]
  plot "read.plt" notitle with linespoints pt 4 lw 3
EOF

tabulka 5 "read"


########### WRITE ################

while read l1 && read -u 5 l2 && read -u 6 l3; do
  echo `sed -re 's/,.*/ /' <<<"$l1"``avg $(sed -re 's/^[^,]*//;s/,/ /g' <<<"$l1")` `avg $(sed -re 's/^[^,]*//;s/,/ /g' <<<"$l2")` `avg $(sed -re 's/^[^,]*//;s/,/ /g' <<<"$l3")`
done <"write.csv" 5<"write_col.csv" 6<"col.csv" >"write.plt"

cat <<EOF | gnuplot
  set terminal pdf
  set output "write.pdf"
  set multiplot
  set title "Zápis"
  set ylabel "Čas [ms]"
  set y2label "Počet restartů [‰ úspěšných transakcí]"
  set xlabel "Počet vláken"
  set yrange [0:]
  set y2range [0:1000]
  set y2tics 0, 100
  set ytics nomirror
  set key bottom right
  plot "write.plt" using 1:2 title "Zápis bez kolizí [ms]" with linespoints pt 4 lw 3, \\
       "write.plt" using 1:3 title "Zápis s kolizemi [ms]" with linespoints pt 5 lw 3, \\
       "write.plt" using 1:4 axes x1y2 title "Kolize [‰]" with linespoints pt 6 lw 3
EOF



tabulka 5 "write"
tabulka 5 "write_col"
tabulka 5 "col"



