glb_filter()           # clears all global filters

glb_filter(GRP_ON)     # sets all global filters

glb_filter(GRP_SM, GRP_AO)   # sets SM-group and AO-group

glb_filter(GRP_ON, -GRP_SC)  # sets all global filters, but clears the SC-group

glb_filter(GRP_QF, "-QS_QF_TICK")  # sets the QF-group, but clears the QS_QF_TICK filter

glb_filter(GRP_AO, 78)` # sets the AO-group and the QS record 78
