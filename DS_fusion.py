"""
Based on: 
"Data Classification Using the Dempster-Shafer Method. 
    Qi Chen, Amanda Whitbrook, Uwe Aickelin and Chris Roadknight"

Dempster’s rule of combination (DRC)
"""

# NOTE replace attributes for model of the ensemble 

# The training data is used to build the mass functions

# 1 maximum and minimum values for each class based 
# on one attribute of the training data are found


# The system combines the mass values from all four attributes using DRC,
# thus producing overall mass values for all hypotheses, and the hypothesis
# with the highest belief value is used to classify the data item. 
# If the hypothesis does not represent a single class, 
# then a second step is necessary.