import matplotlib.pyplot as plt
import numpy as np
from load_MITBIH import *

# Generate graphics for paper

db_path = '/home/mondejar/dataset/ECG/mitdb/m_learning/scikit/'
winL = 90
winR = 90
do_preprocess = True
maxRR = True

use_RR = False
norm_RR = False

reduced_DS = False
leads_flag = [1, 0]
# Load train data 
compute_morph = {'raw'}

label_name = ['N', 'S', 'V', 'F']
line_styles =  ['-', '--', ':', '-.']
l_width = 2


plt.figure(figsize=(8.27, 11.69))

print("1 Raw")
compute_morph = {'raw'}

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))


ax1 = plt.subplot(321)
ax1.title.set_text('(a)')

raw_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    raw_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(raw_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)


# 1 Average wavelets
print("2 wavelets")
compute_morph = {'wvlt'}

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))


ax2 = plt.subplot(322)
ax2.title.set_text('(b)')

wvlt_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    wvlt_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(wvlt_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)


# 2 Average HOS
compute_morph = {'HOS'}
print("3 HOS")

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))


ax3 = plt.subplot(323)
ax3.title.set_text('(c)')

hos_avg = np.zeros((4, features.shape[1]))

for n in range(0,4):
    hos_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(hos_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)

# 3 Average U-LBP1D

compute_morph = {'u-lbp'}
print("4 U LBP")

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))

ax4 = plt.subplot(324)
ax4.title.set_text('(d)')

lbp_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    lbp_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(lbp_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)


"""
compute_morph = {'hbf'}
print("4 HBF")

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))

ax4 = plt.subplot(324)
ax4.title.set_text('d')

hbf_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    hbf_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(hbf_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)
"""

# 4 Average MyMorph
compute_morph = {'myMorph'}
print("5 myMorph")

[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))


ax5 = plt.subplot(325)
ax5.title.set_text('(e)')

myMorph_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    myMorph_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(myMorph_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)

# 5 Average RR intervals
compute_morph = {''}
print("6 RR")

use_RR = True
norm_RR = True
[features_1, labels_1, patient_num_beats_1] = load_mit_db('DS1', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

[features_2, labels_2, patient_num_beats_2] = load_mit_db('DS2', winL, winR, do_preprocess,
    maxRR, use_RR, norm_RR, compute_morph, db_path, reduced_DS, leads_flag)

features = np.vstack((features_1, features_2))
labels = np.concatenate((labels_1, labels_2))


ax6 = plt.subplot(326)
ax6.title.set_text('(f)')

myMorph_avg = np.zeros((4, features.shape[1]))
for n in range(0,4):
    myMorph_avg[n] = np.average(features[labels == n], axis=0)
    plt.plot(myMorph_avg[n], label=label_name[n], linestyle=line_styles[n], linewidth=l_width)

leg = plt.legend(loc='best', ncol=1, shadow=False, fancybox=True)
leg.get_frame().set_alpha(0.5)



#plt.show()

plt.savefig('/home/mondejar/graphic.pdf', dpi=None, facecolor='w', edgecolor='w',
    orientation='portrait', papertype='a4', format='pdf', transparent=True, bbox_inches=None, 
    pad_inches=0.1, frameon=None)

# A4 figsize=(11.69,8.27)