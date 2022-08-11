def on_reset():
    expect_pause()
    glb_filter(GRP_UA)
    current_obj(OBJ_SM_AO, "AO_MyAO")
    continue_test()  # <====
    expect_run()
