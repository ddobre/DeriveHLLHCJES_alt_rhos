rm -r resolution_plots

mkdir resolution_plots
mkdir resolution_plots/logfiles
mkdir resolution_plots/plots
mkdir resolution_plots/resolution
mkdir resolution_plots/response
mkdir resolution_plots/root

python make_histograms_resolution_getnames.py

while read name
do
  nohup python make_histograms_resolution_loop.py --InputTree $name > resolution_plots/logfiles/${name}.log &
done < treelist.txt

rm treelist.txt