# this script convert the full dataset mitdb to .csv

from os import listdir, mkdir, system
from os.path import isfile, isdir, join, exists

dir = '/local/scratch/mondejar/dataset/mitdb/'
#Create folder
csv = dir + 'csv'
if not exists(csv):
	mkdir(csv)

records = [f for f in listdir(dir) if isfile(join(dir, f)) if(f.find('.dat') != -1)]
#print records

for r in records:

	command = 'rdsamp -r ' + r[:-4] + ' -c -H -f 0 -v >' + 'csv/' + r[:-4] + '.csv'
	print(command)
	system(command)

	command_annotations = 'rdann -r ' + r[:-4] +' -f 0 -a atr -v >' + 'csv/' + r[:-4] + 'annotations.txt'
	print(command_annotations)
	system(command_annotations)
