loc_filter(IDS_ALL) # enables all QS-IDs

loc_filter(-IDS_ALL) # disables all QS-IDs

loc_filter(IDS_AO)   # enables all active objcts

loc_filter(IDS_AO, -6)   # enables all active objcts,
                         # but disables AO with priority 6

loc_filter(IDS_AO, IDS_EP)   # enables all active objects and event pools

loc_filter(IDS_AP)       # enables all app-specific QS_IDs

filters = loc_filter(IDS_AO, IDS_EP) # store the 128-bit bitmask returned
