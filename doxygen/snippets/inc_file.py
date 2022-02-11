# file test_inc.pyi (include file)

# common on_reset() callback
def on_reset():
    expect_pause()
    glb_filter(GRP_ALL)
    continue_test()

def my_test_snippet():
    query_curr(OBJ_SM)
    expect("@timestamp Query-SM Obj=Blinky::inst,State=Blinky::off")
