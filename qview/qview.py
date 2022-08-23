#=============================================================================
# QView Monitoring for QP/Spy
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This software is dual-licensed under the terms of the open source GNU
# General Public License version 3 (or any later version), or alternatively,
# under the terms of one of the closed source Quantum Leaps commercial
# licenses.
#
# The terms of the open source GNU General Public License version 3
# can be found at: <www.gnu.org/licenses/gpl-3.0>
#
# The terms of the closed source Quantum Leaps commercial licenses
# can be found at: <www.state-machine.com/licensing>
#
# Redistributions in source code must retain this top-level comment block.
# Plagiarizing this software to sidestep the license obligations is illegal.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#=============================================================================
##
# @date Last updated on: 2022-08-22
# @version Last updated for version: 7.1.0
#
# @file
# @brief QView Monitoring for QP/Spy
# @ingroup qview

import socket
import time
import sys
import struct
import os
import traceback
import webbrowser

from tkinter import *
from tkinter.ttk import * # override the basic Tk widgets with Ttk widgets
from tkinter.simpledialog import *
from struct import pack

#=============================================================================
# QView GUI
# https://www.state-machine.com/qtools/qview.html
#
class QView:
    ## current version of QView
    VERSION = 710

    # public static variables...
    ## menu to be customized
    custom_menu = None

    ## canvas to be customized
    canvas      = None

    # private class variables...
    _text_lines  = "end - 500 lines"
    _attach_dialog = None
    _have_info     = False
    _reset_request = False
    _gui  = None
    _cust = None
    _err  = 0
    _glb_filter = 0x00000000000000000000000000000000
    _loc_filter = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF

    _dtypes = ("8-bit", "16-bit", "32-bit")
    _dsizes = (1, 2, 4)

    @staticmethod
    def _init_gui(root):
        Tk.report_callback_exception = QView._trap_error

        # menus...............................................................
        main_menu = Menu(root, tearoff=0)
        root.config(menu=main_menu)

        # File menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="Save QSPY Dictionaries",
                      command=QView._onSaveDict)
        m.add_command(label="Toggle QSPY Text Output",
                      command=QView._onSaveText)
        m.add_command(label="Toggle QSPY Binary Output",
                      command=QView._onSaveBin)
        m.add_command(label="Toggle Matlab Output",
                      command=QView._onSaveMatlab)
        m.add_command(label="Toggle Sequence Output",
                      command=QView._onSaveSequence)
        m.add_separator()
        m.add_command(label="Exit",  command=QView._quit)
        main_menu.add_cascade(label="File", menu=m)

        # View menu...
        m = Menu(main_menu, tearoff=0)
        QView._view_canvas = IntVar()
        m.add_checkbutton(label="Canvas", variable=QView._view_canvas,
                          command=QView._onCanvasView)
        main_menu.add_cascade(label="View", menu=m)

        # Global-Filters menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="SM Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_SM)
        m.add_command(label="AO Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_AO)
        m.add_command(label="QF Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_QF)
        m.add_command(label="TE Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_TE)
        m.add_command(label="MP Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_MP)
        m.add_command(label="EQ Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_EQ)
        m.add_command(label="SC Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_SC)
        m.add_command(label="SEM Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_SEM)
        m.add_command(label="MTX Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_MTX)
        m.add_separator()
        m.add_command(label="U0 Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_U0)
        m.add_command(label="U1 Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_U1)
        m.add_command(label="U2 Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_U2)
        m.add_command(label="U3 Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_U3)
        m.add_command(label="U4 Group...", accelerator="[NONE]",
             command=QView._onGlbFilter_U4)
        main_menu.add_cascade(label="Global-Filters", menu=m)
        QView._menu_glb_filter = m

        # Local-Filters menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="AO IDs...", accelerator="[NONE]",
            command=QView._onLocFilter_AO)
        m.add_command(label="EP IDs...", accelerator="[NONE]",
            command=QView._onLocFilter_EP)
        m.add_command(label="EQ IDs...", accelerator="[NONE]",
            command=QView._onLocFilter_EQ)
        m.add_command(label="AP IDs...", accelerator="[NONE]",
            command=QView._onLocFilter_AP)
        m.add_separator()
        m.add_command(label="AO-OBJ...", command=QView._onLocFilter_AO_OBJ)
        main_menu.add_cascade(label="Local-Filters", menu=m)
        QView._menu_loc_filter = m

        # Current-Obj menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="SM_OBJ", command=QView._onCurrObj_SM)
        m.add_command(label="AO_OBJ", command=QView._onCurrObj_AO)
        m.add_command(label="MP_OBJ", command=QView._onCurrObj_MP)
        m.add_command(label="EQ_OBJ", command=QView._onCurrObj_EQ)
        m.add_command(label="TE_OBJ", command=QView._onCurrObj_TE)
        m.add_command(label="AP_OBJ", command=QView._onCurrObj_AP)
        m.add_separator()
        m1 = Menu(m, tearoff=0)
        m1.add_command(label="SM_OBJ", command=QView._onQueryCurr_SM)
        m1.add_command(label="AO_OBJ", command=QView._onQueryCurr_AO)
        m1.add_command(label="MP_OBJ", command=QView._onQueryCurr_MP)
        m1.add_command(label="EQ_OBJ", command=QView._onQueryCurr_EQ)
        m1.add_command(label="TE_OBJ", command=QView._onQueryCurr_TE)
        m1.add_command(label="AP_OBJ", command=QView._onQueryCurr_AP)
        m.add_cascade(label="Query Current", menu=m1)
        main_menu.add_cascade(label="Current-Obj", menu=m)
        QView._menu_curr_obj = m

        # Commands menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="Reset Target", command=reset_target)
        m.add_command(label="Query Target Info", command=QView._onTargetInfo)
        m.add_command(label="Tick[0]", command=QView._onTick0)
        m.add_command(label="Tick[1]", command=QView._onTick1)
        m.add_command(label="Command...", command=QView._CommandDialog)
        m.add_separator()
        m.add_command(label="Peek...", command=QView._PeekDialog)
        m.add_command(label="Poke...", command=QView._PokeDialog)
        main_menu.add_cascade(label="Commands", menu=m)
        QView._menu_commands = m

        # Events menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="Publish...",  command=QView._onEvt_PUBLISH)
        m.add_command(label="Post...",     command=QView._onEvt_POST)
        m.add_command(label="Init SM",     command=QView._onEvt_INIT)
        m.add_command(label="Dispatch...", command=QView._onEvt_DISPATCH)
        main_menu.add_cascade(label="Events", menu=m)
        QView._menu_events = m

        # Custom menu...
        m = Menu(main_menu, tearoff=0)
        m.add_separator()
        main_menu.add_cascade(label="Custom", menu=m)
        QView.custom_menu = m

        # Help menu...
        m = Menu(main_menu, tearoff=0)
        m.add_command(label="Online Help", command=QView._onHelp)
        m.add_separator()
        m.add_command(label="About...", command=QView._onAbout)
        main_menu.add_cascade(label="Help", menu=m)

        # statusbar (pack before text-area) ..................................
        QView._scroll_text = IntVar()
        QView._scroll_text.set(1) # text scrolling enabled
        QView._echo_text = IntVar()
        QView._echo_text.set(0) # text echo disabled
        frame = Frame(root, borderwidth=1, relief="raised")
        QView._target = Label(frame, height=2,
                              text="Target: " + QSpy._fmt_target)
        QView._target.pack(side="left")
        c = Checkbutton(frame, text="Scroll", variable=QView._scroll_text)
        c.pack(side="right")
        c = Checkbutton(frame, text="Echo",   variable=QView._echo_text,
                        command=QSpy._reattach)
        c.pack(side="right")
        QView._tx = Label(frame, width=6, anchor=E,
                          borderwidth=1, relief="sunken")
        QView._tx.pack(side="right")
        Label(frame, text="Tx ").pack(side="right")
        QView._rx = Label(frame, width=8, anchor=E,
                          borderwidth=1, relief="sunken")
        QView._rx.pack(side="right")
        Label(frame, text="Rx ").pack(side="right")
        frame.pack(side="bottom", fill="x", pady=0)

        # text-area with scrollbar............................................
        frame = Frame(root, borderwidth=1, relief="sunken")
        scrollbar = Scrollbar(frame)
        QView._text = Text(frame, width=100, height=30,
                           wrap="word", yscrollcommand=scrollbar.set)
        QView._text.bind("<Key>", lambda e: "break") # read-only text
        scrollbar.config(command=QView._text.yview)
        scrollbar.pack(side="right", fill="y")
        QView._text.pack(side="left", fill="both", expand=True)
        frame.pack(side="left", fill="both", expand=True)

        # canvas..............................................................
        QView._canvas_toplevel = Toplevel()
        QView._canvas_toplevel.withdraw() # start not showing
        QView._canvas_toplevel.protocol("WM_DELETE_WINDOW",
                                        QView._onCanvasClose)
        QView._canvas_toplevel.title("QView -- Canvas")
        QView.canvas = Canvas(QView._canvas_toplevel)
        QView.canvas.pack()

        # tkinter variables for dialog boxes .................................
        QView._locAO_OBJ = StringVar()
        QView._currObj   = (StringVar(), StringVar(), StringVar(),
                            StringVar(), StringVar(), StringVar())
        QView._command    = StringVar()
        QView._command_p1 = StringVar()
        QView._command_p2 = StringVar()
        QView._command_p3 = StringVar()
        QView._peek_offs  = StringVar()
        QView._peek_dtype = StringVar(value=QView._dtypes[2])
        QView._peek_len   = StringVar()
        QView._poke_offs  = StringVar()
        QView._poke_dtype = StringVar(value=QView._dtypes[2])
        QView._poke_data  = StringVar()
        QView._evt_act    = StringVar()
        QView._evt_sig    = StringVar()
        QView._evt_par   = (StringVar(), StringVar(), StringVar(),
                            StringVar(), StringVar(), StringVar(),
                            StringVar(), StringVar(), StringVar())
        QView._evt_dtype = (StringVar(), StringVar(), StringVar(),
                            StringVar(), StringVar(), StringVar(),
                            StringVar(), StringVar(), StringVar())
        for i in range(len(QView._evt_par)):
            QView._evt_dtype[i].set(QView._dtypes[2])

        QView._updateMenus()


    # public static functions...

    ## Set QView customization.
    # @param cust the customization class instance
    @staticmethod
    def customize(cust):
        QView._cust = cust

    ## Print a string to the Text area
    @staticmethod
    def print_text(string):
        QView._text.delete(1.0, QView._text_lines)
        QView._text.insert(END, "\n")
        QView._text.insert(END, string)
        if QView._scroll_text.get():
            QView._text.yview_moveto(1) # scroll to the bottom

    ## Make the canvas visible
    # (to be used in the constructor of the customization class)
    @staticmethod
    def show_canvas(view=1):
        QView._view_canvas.set(view)

    # private static functions...
    @staticmethod
    def _quit(err=0):
        QView._err = err
        QView._gui.quit()

    @staticmethod
    def _onExit(*args):
        QView._quit()

    @staticmethod
    def _onReset():
        QView._glb_filter = 0
        QView._loc_filter = QSpy._LOC_FLT_MASK_ALL
        QView._locAO_OBJ.set("")
        for i in range(len(QView._currObj)):
            QView._currObj[i].set("")
        QView._updateMenus()

    @staticmethod
    def _updateMenus():

        # internal helper function
        def _update_glb_filter_menu(label, mask):
            x = (QView._glb_filter & mask)
            if x == 0:
                status = "[ - ]"
            elif x == mask:
                status = "[ + ]"
            else:
                status = "[+-]"
            QView._menu_glb_filter.entryconfig(label,
                    accelerator=status)

        # internal helper function
        def _update_loc_filter_menu(label, mask):
            x = (QView._loc_filter & mask)
            if x == 0:
                status = "[ - ]"
            elif x == mask:
                status = "[ + ]"
            else:
                status = "[+-]"
            QView._menu_loc_filter.entryconfig(label,
                    accelerator=status)

        for i in range(len(QView._currObj)):
            QView._menu_curr_obj.entryconfig(i,
                accelerator=QView._currObj[i].get())
        QView._menu_events.entryconfig(0,
                accelerator=QView._currObj[OBJ_AO].get())
        QView._menu_events.entryconfig(1,
                accelerator=QView._currObj[OBJ_AO].get())
        QView._menu_events.entryconfig(2,
                accelerator=QView._currObj[OBJ_SM].get())
        QView._menu_events.entryconfig(3,
                accelerator=QView._currObj[OBJ_SM].get())
        QView._menu_commands.entryconfig(6,
                accelerator=QView._currObj[OBJ_AP].get())
        QView._menu_commands.entryconfig(7,
                accelerator=QView._currObj[OBJ_AP].get())
        state_SM = "normal"
        state_AO = "normal"
        state_AP = "normal"
        if QView._currObj[OBJ_SM].get() == "":
            state_SM = "disabled"
        if QView._currObj[OBJ_AO].get() == "":
            state_AO = "disabled"
        if QView._currObj[OBJ_AP].get() == "":
            state_AP ="disabled"
        QView._menu_events.entryconfig(0, state=state_AO)
        QView._menu_events.entryconfig(1, state=state_AO)
        QView._menu_events.entryconfig(2, state=state_SM)
        QView._menu_events.entryconfig(3, state=state_SM)
        QView._menu_commands.entryconfig(6, state=state_AP)
        QView._menu_commands.entryconfig(7, state=state_AP)

        _update_glb_filter_menu("SM Group...", QSpy._GLB_FLT_MASK_SM)
        _update_glb_filter_menu("AO Group...", QSpy._GLB_FLT_MASK_AO)
        _update_glb_filter_menu("QF Group...", QSpy._GLB_FLT_MASK_QF)
        _update_glb_filter_menu("TE Group...", QSpy._GLB_FLT_MASK_TE)
        _update_glb_filter_menu("MP Group...", QSpy._GLB_FLT_MASK_MP)
        _update_glb_filter_menu("EQ Group...", QSpy._GLB_FLT_MASK_EQ)
        _update_glb_filter_menu("SC Group...", QSpy._GLB_FLT_MASK_SC)
        _update_glb_filter_menu("SEM Group...", QSpy._GLB_FLT_MASK_SEM)
        _update_glb_filter_menu("MTX Group...", QSpy._GLB_FLT_MASK_MTX)
        _update_glb_filter_menu("U0 Group...", QSpy._GLB_FLT_MASK_U0)
        _update_glb_filter_menu("U1 Group...", QSpy._GLB_FLT_MASK_U1)
        _update_glb_filter_menu("U2 Group...", QSpy._GLB_FLT_MASK_U2)
        _update_glb_filter_menu("U3 Group...", QSpy._GLB_FLT_MASK_U3)
        _update_glb_filter_menu("U4 Group...", QSpy._GLB_FLT_MASK_U4)

        _update_loc_filter_menu("AO IDs...", QSpy._LOC_FLT_MASK_AO)
        _update_loc_filter_menu("EP IDs...", QSpy._LOC_FLT_MASK_EP)
        _update_loc_filter_menu("EQ IDs...", QSpy._LOC_FLT_MASK_EQ)
        _update_loc_filter_menu("AP IDs...", QSpy._LOC_FLT_MASK_AP)
        QView._menu_loc_filter.entryconfig("AO-OBJ...",
                accelerator=QView._locAO_OBJ.get())


    @staticmethod
    def _trap_error(*args):
        QView._showerror("Runtime Error",
           traceback.format_exc(3))
        QView._quit(-3)

    @staticmethod
    def _assert(cond, message):
        if not cond:
            QView._showerror("Assertion",
               message)
        QView._quit(-3)

    @staticmethod
    def _onSaveDict(*args):
        QSpy._sendTo(pack("<B", QSpy._QSPY_SAVE_DICT))

    @staticmethod
    def _onSaveText(*args):
        QSpy._sendTo(pack("<B", QSpy._QSPY_SCREEN_OUT))

    @staticmethod
    def _onSaveBin(*args):
        QSpy._sendTo(pack("<B", QSpy._QSPY_BIN_OUT))

    @staticmethod
    def _onSaveMatlab(*args):
        QSpy._sendTo(pack("<B", QSpy._QSPY_MATLAB_OUT))

    @staticmethod
    def _onSaveSequence(*args):
        QSpy._sendTo(pack("<B", QSpy._QSPY_SEQUENCE_OUT))

    @staticmethod
    def _onCanvasView(*args):
        if QView._view_canvas.get():
            QView._canvas_toplevel.state("normal")
            # make the canvas jump to the front
            QView._canvas_toplevel.attributes("-topmost", 1)
            QView._canvas_toplevel.attributes("-topmost", 0)
        else:
            QView._canvas_toplevel.withdraw()

    @staticmethod
    def _onCanvasClose():
        QView._view_canvas.set(0)
        QView._canvas_toplevel.withdraw()

    @staticmethod
    def _onGlbFilter_SM(*args):
        QView._GlbFilterDialog("SM Group", QSpy._GLB_FLT_MASK_SM)

    @staticmethod
    def _onGlbFilter_AO(*args):
        QView._GlbFilterDialog("AO Group", QSpy._GLB_FLT_MASK_AO)

    @staticmethod
    def _onGlbFilter_QF(*args):
        QView._GlbFilterDialog("QF Group", QSpy._GLB_FLT_MASK_QF)

    @staticmethod
    def _onGlbFilter_TE(*args):
        QView._GlbFilterDialog("TE Group", QSpy._GLB_FLT_MASK_TE)

    @staticmethod
    def _onGlbFilter_EQ(*args):
        QView._GlbFilterDialog("EQ Group", QSpy._GLB_FLT_MASK_EQ)

    @staticmethod
    def _onGlbFilter_MP(*args):
        QView._GlbFilterDialog("MP Group", QSpy._GLB_FLT_MASK_MP)

    @staticmethod
    def _onGlbFilter_SC(*args):
        QView._GlbFilterDialog("SC Group", QSpy._GLB_FLT_MASK_SC)

    @staticmethod
    def _onGlbFilter_SEM(*args):
        QView._GlbFilterDialog("SEM Group", QSpy._GLB_FLT_MASK_SEM)

    @staticmethod
    def _onGlbFilter_MTX(*args):
        QView._GlbFilterDialog("MTX Group", QSpy._GLB_FLT_MASK_MTX)

    @staticmethod
    def _onGlbFilter_U0(*args):
        QView._GlbFilterDialog("U0 Group", QSpy._GLB_FLT_MASK_U0)

    @staticmethod
    def _onGlbFilter_U1(*args):
        QView._GlbFilterDialog("U1 Group", QSpy._GLB_FLT_MASK_U1)

    @staticmethod
    def _onGlbFilter_U2(*args):
        QView._GlbFilterDialog("U2 Group", QSpy._GLB_FLT_MASK_U2)

    @staticmethod
    def _onGlbFilter_U3(*args):
        QView._GlbFilterDialog("U3 Group", QSpy._GLB_FLT_MASK_U3)

    @staticmethod
    def _onGlbFilter_U4(*args):
        QView._GlbFilterDialog("U4 Group", QSpy._GLB_FLT_MASK_U4)

    @staticmethod
    def _onLocFilter_AO(*args):
        QView._LocFilterDialog("AO IDs", QSpy._LOC_FLT_MASK_AO)

    @staticmethod
    def _onLocFilter_EP(*args):
        QView._LocFilterDialog("EP IDs", QSpy._LOC_FLT_MASK_EP)

    @staticmethod
    def _onLocFilter_EQ(*args):
        QView._LocFilterDialog("EQ IDs", QSpy._LOC_FLT_MASK_EQ)

    @staticmethod
    def _onLocFilter_AP(*args):
        QView._LocFilterDialog("AP IDs", QSpy._LOC_FLT_MASK_AP)

    @staticmethod
    def _onLocFilter_AO_OBJ(*args):
        QView._LocFilterDialog_AO_OBJ()

    @staticmethod
    def _onCurrObj_SM(*args):
        QView._CurrObjDialog(OBJ_SM, "SM_OBJ")
        QView._updateMenus()

    @staticmethod
    def _onCurrObj_AO(*args):
        QView._CurrObjDialog(OBJ_AO, "AO_OBJ")
        QView._updateMenus()

    @staticmethod
    def _onCurrObj_MP(*args):
        QView._CurrObjDialog(OBJ_MP, "MP_OBJ")

    @staticmethod
    def _onCurrObj_EQ(*args):
        QView._CurrObjDialog(OBJ_EQ, "EQ_OBJ")

    @staticmethod
    def _onCurrObj_TE(*args):
        QView._CurrObjDialog(OBJ_TE, "TE_OBJ")

    @staticmethod
    def _onCurrObj_AP(*args):
        QView._CurrObjDialog(OBJ_AP, "AP_OBJ")
        QView._updateMenus()

    @staticmethod
    def _onQueryCurr_SM(*args):
        query_curr(OBJ_SM)

    @staticmethod
    def _onQueryCurr_AO(*args):
        query_curr(OBJ_AO)

    @staticmethod
    def _onQueryCurr_MP(*args):
        query_curr(OBJ_MP)

    @staticmethod
    def _onQueryCurr_EQ(*args):
        query_curr( OBJ_EQ)

    @staticmethod
    def _onQueryCurr_TE(*args):
        query_curr(OBJ_TE)

    @staticmethod
    def _onQueryCurr_AP(*args):
        query_curr(OBJ_AP)

    @staticmethod
    def _onTargetInfo(*args):
        QSpy._sendTo(pack("<B", QSpy._TRGT_INFO))

    @staticmethod
    def _onTick0(*args):
        tick(0)

    @staticmethod
    def _onTick1(*args):
        tick(1)

    @staticmethod
    def _onEvt_PUBLISH(*args):
        QView._EvtDialog("Publish Event", publish)

    @staticmethod
    def _onEvt_POST(*args):
        QView._EvtDialog("Post Event", post)

    @staticmethod
    def _onEvt_INIT(*args):
        QView._EvtDialog("Init Event", init)

    @staticmethod
    def _onEvt_DISPATCH(*args):
        QView._EvtDialog("Dispatch Event", dispatch)

    @staticmethod
    def _onHelp(*args):
        webbrowser.open("https://www.state-machine.com/qtools/qview.html",
                        new=2)

    @staticmethod
    def _onAbout(*args):
        QView._MessageDialog("About QView",
            "QView version   " + \
            "%d.%d.%d"%(QView.VERSION//100, \
                       (QView.VERSION//10) % 10, \
                        QView.VERSION % 10) + \
            "\n\nFor more information see:\n"
            "https://www.state-machine.com/qtools/qview.html")

    @staticmethod
    def _showerror(title, message):
        QView._gui.after_cancel(QSpy._after_id)
        QView._MessageDialog(title, message)

    @staticmethod
    def _strVar_value(strVar, base=0):
        str = strVar.get().replace(" ", "") # cleanup spaces
        strVar.set(str)
        try:
            value = int(str, base=base)
            return value # integer
        except:
            return str # string


    #-------------------------------------------------------------------------
    # private dialog boxes...
    #
    class _AttachDialog(Dialog):
        def __init__(self):
            QView._attach_dialog = self
            super().__init__(QView._gui, "Attach to QSpy")

        def body(self, master):
            self.resizable(height=False, width=False)
            Label(master,
                     text="Make sure that QSPY back-end is running and\n"
                     "not already used by other front-end.\n\n"
                     "Press Attach to re-try to attach or\n"
                     "Close to quit.").pack()

        def buttonbox(self):
            box = Frame(self)
            w = Button(box, text="Attach", width=10, command=self.ok,
                       default=ACTIVE)
            w.pack(side=LEFT, padx=5, pady=5)
            w = Button(box, text="Close", width=10, command=self.cancel)
            w.pack(side=LEFT, padx=5, pady=5)
            self.bind("<Return>", self.ok)
            self.bind("<Escape>", self.cancel)
            box.pack()

        def close(self):
            super().cancel()
            QView._attach_dialog = None

        def validate(self):
            QSpy._attach()
            return 0

        def apply(self):
            QView._attach_dialog = None

        def cancel(self, event=None):
            super().cancel()
            QView._quit()


    #.........................................................................
    # helper dialog box for message boxes, @sa QView._showerror()
    class _MessageDialog(Dialog):
        def __init__(self, title, message):
            self.message = message
            super().__init__(QView._gui, title)

        def body(self, master):
            self.resizable(height=False, width=False)
            Label(master, text=self.message, justify=LEFT).pack()

        def buttonbox(self):
            box = Frame(self)
            Button(box, text="OK", width=10, command=self.ok,
                   default=ACTIVE).pack(side=LEFT, padx=5, pady=5)
            self.bind("<Return>", self.ok)
            box.pack()

    #.........................................................................
    class _GlbFilterDialog(Dialog):
        def __init__(self, title, mask):
            self._title = title
            self._mask = mask
            super().__init__(QView._gui, title)

        def body(self, master):
            N_ROW = 3
            Button(master, text="Select ALL", command=self._sel_all)\
                .grid(row=0,column=0, padx=2, pady=2, sticky=W+E)
            Button(master, text="Clear ALL", command=self._clr_all)\
                .grid(row=0,column=N_ROW-1, padx=2, pady=2, sticky=W+E)
            n = 0
            self._filter_var = []
            for i in range(QSpy._GLB_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var.append(IntVar())
                    if QView._glb_filter & (1 << i):
                        self._filter_var[n].set(1)
                    Checkbutton(master, text=QSpy._QS[i], anchor=W,
                                variable=self._filter_var[n])\
                        .grid(row=(n + N_ROW)//N_ROW,column=(n+N_ROW)%N_ROW,
                              padx=2,pady=2,sticky=W)
                    n += 1

        def _sel_all(self):
            n = 0
            for i in range(QSpy._GLB_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var[n].set(1)
                    n += 1

        def _clr_all(self):
            n = 0
            for i in range(QSpy._GLB_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var[n].set(0)
                    n += 1

        def apply(self):
            n = 0
            for i in range(QSpy._GLB_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    if self._filter_var[n].get():
                        QView._glb_filter |= (1 << i)
                    else:
                        QView._glb_filter &= ~(1 << i)
                    n += 1
            QSpy._sendTo(pack("<BBQQ", QSpy._TRGT_GLB_FILTER, 16,
                        QView._glb_filter & 0xFFFFFFFFFFFFFFFF,
                        QView._glb_filter >> 64))
            QView._updateMenus()

    #.........................................................................
    class _LocFilterDialog(Dialog):
        def __init__(self, title, mask):
            self._title = title
            self._mask = mask
            super().__init__(QView._gui, title)

        def body(self, master):
            N_ROW = 8
            Button(master, text="Select ALL", command=self._sel_all)\
                .grid(row=0,column=0, padx=2, pady=2, sticky=W+E)
            Button(master, text="Clear ALL", command=self._clr_all)\
                .grid(row=0,column=N_ROW-1, padx=2, pady=2, sticky=W+E)
            n = 0
            self._filter_var = []
            if self._mask == QSpy._LOC_FLT_MASK_AO:
                QS_id = "AO-prio=%d"
            else:
                QS_id = "QS-ID=%d"
            for i in range(QSpy._LOC_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var.append(IntVar())
                    if QView._loc_filter & (1 << i):
                        self._filter_var[n].set(1)
                    Checkbutton(master, text=QS_id%(i), anchor=W,
                                variable=self._filter_var[n])\
                        .grid(row=(n + N_ROW)//N_ROW,column=(n+N_ROW)%N_ROW,
                              padx=2,pady=2,sticky=W)
                    n += 1

        def _sel_all(self):
            n = 0
            for i in range(QSpy._LOC_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var[n].set(1)
                    n += 1

        def _clr_all(self):
            n = 0
            for i in range(QSpy._LOC_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    self._filter_var[n].set(0)
                    n += 1

        def apply(self):
            n = 0
            for i in range(QSpy._LOC_FLT_RANGE):
                if self._mask & (1 << i) != 0:
                    if self._filter_var[n].get():
                        QView._loc_filter |= (1 << i)
                    else:
                        QView._loc_filter &= ~(1 << i)
                    n += 1
            QSpy._sendTo(pack("<BBQQ", QSpy._TRGT_LOC_FILTER, 16,
                        QView._loc_filter & 0xFFFFFFFFFFFFFFFF,
                        QView._loc_filter >> 64))
            QView._updateMenus()

    #.........................................................................
    # deprecated
    class _LocFilterDialog_AO_OBJ(Dialog):
        def __init__(self):
            super().__init__(QView._gui, "Local AO-OBJ Filter")

        def body(self, master):
            Label(master, text="AO-OBJ").grid(row=0,column=0,
                sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._locAO_OBJ).grid(row=0,column=1)

        def validate(self):
            self._obj = QView._strVar_value(QView._locAO_OBJ)
            return 1

        def apply(self):
            ao_filter(self._obj)

    #.........................................................................
    class _CurrObjDialog(Dialog):
        def __init__(self, obj_kind, label):
            self._obj_kind = obj_kind
            self._label = label
            super().__init__(QView._gui, "Current Object")

        def body(self, master):
            Label(master, text=self._label).grid(row=0,column=0,
                sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._currObj[self._obj_kind])\
                     .grid(row=0,column=1)

        def validate(self):
            self._obj = QView._strVar_value(QView._currObj[self._obj_kind])
            if self._obj == "":
                self._obj = 0
            return 1

        def apply(self):
            current_obj(self._obj_kind, self._obj)

    #.........................................................................
    class _CommandDialog(Dialog):
        def __init__(self):
            super().__init__(QView._gui, "Command")
        def body(self, master):
            Label(master, text="command").grid(row=0,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._command).grid(row=0,column=1,pady=2)
            Label(master, text="param1").grid(row=1,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._command_p1).grid(row=1,column=1,padx=2)
            Label(master, text="param2").grid(row=2,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._command_p2).grid(row=2,column=1,padx=2)
            Label(master, text="param3").grid(row=3,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._command_p3).grid(row=3,column=1,padx=2)

        def validate(self):
            self._cmdId = QView._strVar_value(QView._command)
            if self._cmdId == "":
                QView._MessageDialog("Command Error", "empty command")
                return 0
            self._param1 = QView._strVar_value(QView._command_p1)
            if self._param1 == "":
                self._param1 = 0
            elif not isinstance(self._param1, int):
                QView._MessageDialog("Command Error", "param1 not integer")
                return 0
            self._param2 = QView._strVar_value(QView._command_p2)
            if self._param2 == "":
                self._param2 = 0
            elif not isinstance(self._param2, int):
                QView._MessageDialog("Command Error", "param2 not integer")
                return 0
            self._param3 = QView._strVar_value(QView._command_p3)
            if self._param3 == "":
                self._param3 = 0
            elif not isinstance(self._param3, int):
                QView._MessageDialog("Command Error", "param3 not integer")
                return 0
            return 1

        def apply(self):
            command(self._cmdId, self._param1, self._param2, self._param3)

    #.........................................................................
    class _PeekDialog(Dialog):
        def __init__(self):
            super().__init__(QView._gui, "Peek")

        def body(self, master):
            Label(master, text="obj/addr").grid(row=0,column=0,
                sticky=E,padx=2)
            Label(master, text=QView._currObj[OBJ_AP].get(),anchor=W,
                relief=SUNKEN).grid(row=0,column=1, columnspan=2,sticky=E+W)
            Label(master, text="offset").grid(row=1,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                textvariable=QView._peek_offs).grid(row=1,column=1,
                columnspan=2)
            Label(master, text="n-units").grid(row=2,column=0,
                sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=12,
                textvariable=QView._peek_len).grid(row=2,column=1,
                sticky=E+W,padx=2)
            OptionMenu(master, QView._peek_dtype, *QView._dtypes).grid(row=2,
                column=2,sticky=E,padx=2)

        def validate(self):
            if QView._currObj[OBJ_AP].get() == "":
                QView._MessageDialog("Peek Error", "Current AP_OBJ not set")
                return 0
            self._offs = QView._strVar_value(QView._peek_offs)
            if not isinstance(self._offs, int):
                self._offs = 0
            i = QView._dtypes.index(QView._peek_dtype.get())
            self._size = QView._dsizes[i]
            self._len = QView._strVar_value(QView._peek_len)
            if not isinstance(self._len, int):
                self._len = 1
            return 1

        def apply(self):
            peek(self._offs, self._size, self._len)

    #.........................................................................
    class _PokeDialog(Dialog):
        def __init__(self):
            super().__init__(QView._gui, "Poke")

        def body(self, master):
            Label(master, text="obj/addr").grid(row=0,column=0,sticky=E,padx=2)
            Label(master, text=QView._currObj[OBJ_AP].get(), anchor=W,
                  relief=SUNKEN).grid(row=0,column=1,sticky=E+W)
            Label(master, text="offset").grid(row=1,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._poke_offs).grid(row=1,column=1)
            OptionMenu(master, QView._poke_dtype,
                       *QView._dtypes).grid(row=2,column=0,sticky=E,padx=2)
            Entry(master, relief=SUNKEN, width=25,
                  textvariable=QView._poke_data).grid(row=2,column=1)

        def validate(self):
            if QView._currObj[OBJ_AP].get() == "":
                QView._MessageDialog("Poke Error", "Current AP_OBJ not set")
                return 0
            self._offs = QView._strVar_value(QView._poke_offs)
            if not isinstance(self._offs, int):
                self._offs = 0

            self._data = QView._strVar_value(QView._poke_data)
            if not isinstance(self._data, int):
                QView._MessageDialog("Poke Error", "data not integer")
                return 0
            dtype = QView._poke_dtype.get()
            self._size = QView._dsizes[QView._dtypes.index(dtype)]
            if self._size == 1 and self._data > 0xFF:
                QView._MessageDialog("Poke Error", "8-bit data out of range")
                return 0
            if self._size == 2 and self._data > 0xFFFF:
                QView._MessageDialog("Poke Error", "16-bit data out of range")
                return 0
            if self._size == 4 and self._data > 0xFFFFFFFF:
                QView._MessageDialog("Poke Error", "32-bit data out of range")
                return 0
            return 1

        def apply(self):
            poke(self._offs, self._size, self._data)

    #.........................................................................
    class _EvtDialog(Dialog):
        def __init__(self, title, action):
            self._action = action
            if action == dispatch:
                self._obj = QView._currObj[OBJ_SM].get()
            else:
                self._obj = QView._currObj[OBJ_AO].get()
            super().__init__(QView._gui, title)

        def body(self, master):
            Label(master, text="obj/addr").grid(row=0,column=0,
                  sticky=E,padx=2)
            Label(master, text=self._obj, anchor=W,
                  relief=SUNKEN).grid(row=0,column=1,columnspan=2,sticky=E+W)
            Frame(master,height=2,borderwidth=1,relief=SUNKEN).grid(row=1,
                  column=0,columnspan=3,sticky=E+W+N+S,pady=4)
            Label(master, text="sig").grid(row=2,column=0,sticky=E,
                  padx=2,pady=4)
            Entry(master, relief=SUNKEN,
                  textvariable=QView._evt_sig).grid(row=2,column=1,
                  columnspan=2,sticky=E+W)
            for i in range(len(QView._evt_par)):
                Label(master, text="par%d"%(i+1)).grid(row=3+i,column=0,
                      sticky=E,padx=2)
                OptionMenu(master, QView._evt_dtype[i], *QView._dtypes).grid(
                      row=3+i,column=1,sticky=E,padx=2)
                Entry(master, relief=SUNKEN, width=18,
                      textvariable=QView._evt_par[i]).grid(row=3+i,column=2,
                      sticky=E+W)

        def validate(self):
            self._sig = QView._strVar_value(QView._evt_sig)
            if self._sig == "":
                QView._MessageDialog("Event error", "empty event sig")
                return 0
            self._params = bytearray()
            for i in range(len(QView._evt_par)):
                par = QView._strVar_value(QView._evt_par[i])
                if par == "":
                    break;
                if not isinstance(par, int):
                    QView._MessageDialog("Event Error: par%d"%(i),
                                         "data not integer")
                    return 0
                idx = QView._dtypes.index(QView._evt_dtype[i].get())
                size = QView._dsizes[idx]
                if size == 1 and par > 0xFF:
                    QView._MessageDialog("Event Error: par%d"%(i),
                                         "8-bit data out of range")
                    return 0
                if size == 2 and par > 0xFFFF:
                    QView._MessageDialog("Event Error: par%d"%(i),
                                         "16-bit data out of range")
                    return 0
                if size == 4 and par > 0xFFFFFFFF:
                    QView._MessageDialog("Event Error: par%d"%(i),
                                         "32-bit data out of range")
                    return 0

                fmt = QSpy._fmt_endian + ("B", "H", "I")[idx]
                self._params.extend(pack(fmt, par))
            return 1

        def apply(self):
            self._action(self._sig, self._params)


#=============================================================================
## Helper class for UDP-communication with the QSpy front-end
# (non-blocking UDP-socket version for QView)
#
class QSpy:
    # private class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    _rx_seq = 0
    _host_addr = ["localhost", 7701] # list, to be converted to a tuple
    _local_port = 0 # let the OS decide the best local port
    _after_id = None

    # formats of various packet elements from the Target
    _fmt_target    = "UNKNOWN"
    _fmt_endian    = "<"
    _size_objPtr   = 4
    _size_funPtr   = 4
    _size_tstamp   = 4
    _size_sig      = 2
    _size_evtSize  = 2
    _size_evtSize  = 2
    _size_queueCtr = 1
    _size_poolCtr  = 2
    _size_poolBlk  = 2
    _size_tevtCtr  = 2
    _fmt = "xBHxLxxxQ"

    # QSPY UDP socket poll interval [ms]
    # NOTE: the chosen value actually sleeps for one system clock tick,
    # which is typically 10ms
    _POLLI = 10

    # tuple of QS records from the Target.
    # !!! NOTE: Must match qs_copy.h !!!
    _QS = ("QS_EMPTY",
        # [1] SM records
        "QS_QEP_STATE_ENTRY",     "QS_QEP_STATE_EXIT",
        "QS_QEP_STATE_INIT",      "QS_QEP_INIT_TRAN",
        "QS_QEP_INTERN_TRAN",     "QS_QEP_TRAN",
        "QS_QEP_IGNORED",         "QS_QEP_DISPATCH",
        "QS_QEP_UNHANDLED",

        # [10] Active Object (AO) records
        "QS_QF_ACTIVE_DEFER",     "QS_QF_ACTIVE_RECALL",
        "QS_QF_ACTIVE_SUBSCRIBE", "QS_QF_ACTIVE_UNSUBSCRIBE",
        "QS_QF_ACTIVE_POST",      "QS_QF_ACTIVE_POST_LIFO",
        "QS_QF_ACTIVE_GET",       "QS_QF_ACTIVE_GET_LAST",
        "QS_QF_ACTIVE_RECALL_ATTEMPT",

        # [19] Event Queue (EQ) records
        "QS_QF_EQUEUE_POST",      "QS_QF_EQUEUE_POST_LIFO",
        "QS_QF_EQUEUE_GET",       "QS_QF_EQUEUE_GET_LAST",

        # [23] Framework (QF) records
        "QS_QF_NEW_ATTEMPT",

        # [24] Memory Pool (MP) records
        "QS_QF_MPOOL_GET",        "QS_QF_MPOOL_PUT",

        # [26] Additional Framework (QF) records
        "QS_QF_PUBLISH",          "QS_QF_NEW_REF",
        "QS_QF_NEW",              "QS_QF_GC_ATTEMPT",
        "QS_QF_GC",               "QS_QF_TICK",

        # [32] Time Event (TE) records
        "QS_QF_TIMEEVT_ARM",      "QS_QF_TIMEEVT_AUTO_DISARM",
        "QS_QF_TIMEEVT_DISARM_ATTEMPT", "QS_QF_TIMEEVT_DISARM",
        "QS_QF_TIMEEVT_REARM",    "QS_QF_TIMEEVT_POST",

        # [38] Additional (QF) records
        "QS_QF_DELETE_REF",       "QS_QF_CRIT_ENTRY",
        "QS_QF_CRIT_EXIT",        "QS_QF_ISR_ENTRY",
        "QS_QF_ISR_EXIT",         "QS_QF_INT_DISABLE",
        "QS_QF_INT_ENABLE",

        # [45] Additional Active Object (AO) records
        "QS_QF_ACTIVE_POST_ATTEMPT",

        # [46] Additional Event Queue (EQ) records
        "QS_QF_EQUEUE_POST_ATTEMPT",

        # [47] Additional Memory Pool (MP) records
        "QS_QF_MPOOL_GET_ATTEMPT",

        # [48] old Mutex records (deprecated in QP 7.1.0)
        "QS_MUTEX_LOCK",          "QS_MUTEX_UNLOCK",

        # [50] Scheduler (SC) records
        "QS_SCHED_LOCK",          "QS_SCHED_UNLOCK",
        "QS_SCHED_NEXT",          "QS_SCHED_IDLE",
        "QS_SCHED_RESUME",

        # [55] Additional QEP records
        "QS_QEP_TRAN_HIST",       "QS_QEP_TRAN_EP",
        "QS_QEP_TRAN_XP",

        # [58] Miscellaneous QS records (not maskable)
        "QS_TEST_PAUSED",         "QS_TEST_PROBE_GET",
        "QS_SIG_DICT",            "QS_OBJ_DICT",
        "QS_FUN_DICT",            "QS_USR_DICT",
        "QS_TARGET_INFO",         "QS_TARGET_DONE",
        "QS_RX_STATUS",           "QS_QUERY_DATA",
        "QS_PEEK_DATA",           "QS_ASSERT_FAIL",
        "QS_QF_RUN",

        # [71] Semaphore (SEM) records
        "QS_SEM_TAKE",            "QS_SEM_BLOCK",
        "QS_SEM_SIGNAL",          "QS_SEM_BLOCK_ATTEMPT",

        # [75] Mutex (MTX) records
        "QS_MTX_LOCK",            "QS_MTX_BLOCK",
        "QS_MTX_UNLOCK",          "QS_MTX_LOCK_ATTEMPT",
        "QS_MTX_BLOCK_ATTEMPT",   "QS_MTX_UNLOCK_ATTEMPT",

        # [81] Reserved QS records
        "QS_RESERVED_81",
        "QS_RESERVED_82",         "QS_RESERVED_83",
        "QS_RESERVED_84",         "QS_RESERVED_85",
        "QS_RESERVED_86",         "QS_RESERVED_87",
        "QS_RESERVED_88",         "QS_RESERVED_89",
        "QS_RESERVED_90",         "QS_RESERVED_91",
        "QS_RESERVED_92",         "QS_RESERVED_93",
        "QS_RESERVED_94",         "QS_RESERVED_95",
        "QS_RESERVED_96",         "QS_RESERVED_97",
        "QS_RESERVED_98",         "QS_RESERVED_99",

        # [100] Application-specific (User) QS records
        "QS_USER_00",             "QS_USER_01",
        "QS_USER_02",             "QS_USER_03",
        "QS_USER_04",             "QS_USER_05",
        "QS_USER_06",             "QS_USER_07",
        "QS_USER_08",             "QS_USER_09",
        "QS_USER_10",             "QS_USER_11",
        "QS_USER_12",             "QS_USER_13",
        "QS_USER_14",             "QS_USER_15",
        "QS_USER_16",             "QS_USER_17",
        "QS_USER_18",             "QS_USER_19",
        "QS_USER_20",             "QS_USER_21",
        "QS_USER_22",             "QS_USER_23",
        "QS_USER_24")

    # global filter masks
    _GLB_FLT_MASK_ALL= 0x1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    _GLB_FLT_MASK_SM = 0x000000000000000003800000000003FE
    _GLB_FLT_MASK_AO = 0x0000000000000000000020000007FC00
    _GLB_FLT_MASK_QF = 0x000000000000000000001FC0FC800000
    _GLB_FLT_MASK_TE = 0x00000000000000000000003F00000000
    _GLB_FLT_MASK_EQ = 0x00000000000000000000400000780000
    _GLB_FLT_MASK_MP = 0x00000000000000000000800003000000
    _GLB_FLT_MASK_SC = 0x0000000000000000007C000000000000
    _GLB_FLT_MASK_SEM= 0x00000000000007800000000000000000
    _GLB_FLT_MASK_MTX= 0x000000000001F8000000000000000000
    _GLB_FLT_MASK_U0 = 0x000001F0000000000000000000000000
    _GLB_FLT_MASK_U1 = 0x00003E00000000000000000000000000
    _GLB_FLT_MASK_U2 = 0x0007C000000000000000000000000000
    _GLB_FLT_MASK_U3 = 0x00F80000000000000000000000000000
    _GLB_FLT_MASK_U4 = 0x1F000000000000000000000000000000
    _GLB_FLT_MASK_UA = 0x1FFFFFF0000000000000000000000000
    _GLB_FLT_RANGE  = 125

    # local filter masks
    _LOC_FLT_MASK_ALL= 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    _LOC_FLT_MASK_AO = 0x0000000000000001FFFFFFFFFFFFFFFE
    _LOC_FLT_MASK_EP = 0x000000000000FFFE0000000000000000
    _LOC_FLT_MASK_EQ = 0x00000000FFFF00000000000000000000
    _LOC_FLT_MASK_AP = 0xFFFFFFFF000000000000000000000000
    _LOC_FLT_RANGE  = 128

    # interesting packets from QSPY/Target...
    _PKT_TEXT_ECHO   = 0
    _PKT_TARGET_INFO = 64
    _PKT_ASSERTION   = 69
    _PKT_QF_RUN      = 70
    _PKT_ATTACH_CONF = 128
    _PKT_DETACH      = 129

    # records to the Target...
    _TRGT_INFO       = 0
    _TRGT_COMMAND    = 1
    _TRGT_RESET      = 2
    _TRGT_TICK       = 3
    _TRGT_PEEK       = 4
    _TRGT_POKE       = 5
    _TRGT_FILL       = 6
    _TRGT_TEST_SETUP = 7
    _TRGT_TEST_TEARDOWN = 8
    _TRGT_TEST_PROBE = 9
    _TRGT_GLB_FILTER = 10
    _TRGT_LOC_FILTER = 11
    _TRGT_AO_FILTER  = 12
    _TRGT_CURR_OBJ   = 13
    _TRGT_CONTINUE   = 14
    _TRGT_QUERY_CURR = 15
    _TRGT_EVENT      = 16

    # packets to QSpy only...
    _QSPY_ATTACH       = 128
    _QSPY_DETACH       = 129
    _QSPY_SAVE_DICT    = 130
    _QSPY_SCREEN_OUT   = 131
    _QSPY_BIN_OUT      = 132
    _QSPY_MATLAB_OUT   = 133
    _QSPY_SEQUENCE_OUT = 134

    # packets to QSpy to be "massaged" and forwarded to the Target...
    _QSPY_SEND_EVENT      = 135
    _QSPY_SEND_AO_FILTER  = 136
    _QSPY_SEND_CURR_OBJ   = 137
    _QSPY_SEND_COMMAND    = 138
    _QSPY_SEND_TEST_PROBE = 139

    # special event sub-commands for QSPY_SEND_EVENT
    _EVT_PUBLISH   = 0
    _EVT_POST      = 253
    _EVT_INIT      = 254
    _EVT_DISPATCH  = 255

    @staticmethod
    def _init():
        # Create socket
        QSpy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        QSpy._sock.setblocking(0) # NON-BLOCKING socket
        try:
            QSpy._sock.bind(("0.0.0.0", QSpy._local_port))
            #print("bind: ", ("0.0.0.0", QSpy._local_port))
        except:
            QView._showerror("UDP Socket Error",
               "Can't bind the UDP socket\n"
               "to the specified local_host.\n"
               "Check if other instances of qspyview\n"
               "or qutest are running...")
            QView._quit(-1)
            return -1
        return 0

    @staticmethod
    def _attach():
        QSpy._is_attached = False
        QView._have_info  = False
        if QView._echo_text.get():
            channels = 0x3
        else:
            channels = 0x1
        QSpy._sendTo(pack("<BB", QSpy._QSPY_ATTACH, channels))
        QSpy._attach_ctr = 50
        QSpy._after_id = QView._gui.after(1, QSpy._poll0) # start poll0

    @staticmethod
    def _detach():
        if QSpy._sock is None:
            return
        QSpy._sendTo(pack("<B", QSpy._QSPY_DETACH))
        time.sleep(0.25) # let the socket finish sending the packet
        #QSpy._sock.shutdown(socket.SHUT_RDWR)
        QSpy._sock.close()
        QSpy._sock = None

    @staticmethod
    def _reattach():
        # channels: 0x1-binary, 0x2-text, 0x3-both
        if QView._echo_text.get():
            channels = 0x3
        else:
            channels = 0x1
        QSpy._sendTo(pack("<BB", QSpy._QSPY_ATTACH, channels))

    # poll the UDP socket until the QSpy confirms ATTACH
    @staticmethod
    def _poll0():
        #print("poll0 ", QSpy._attach_ctr)
        QSpy._attach_ctr -= 1
        if QSpy._attach_ctr == 0:
            if QView._attach_dialog is None:
                QView._AttachDialog() # launch the AttachDialog
            return

        try:
            packet = QSpy._sock.recv(4096)
            if not packet:
                QView._showerror("UDP Socket Error",
                   "Connection closed by QSpy")
                QView._quit(-1)
                return
        except OSError: # non-blocking socket...
            QSpy._after_id = QView._gui.after(QSpy._POLLI, QSpy._poll0)
            return # <======== most frequent return (no packet)
        except:
            QView._showerror("UDP Socket Error",
               "Uknown UDP socket error")
            QView._quit(-1)
            return

        # parse the packet...
        dlen = len(packet)
        if dlen < 2:
            QView._showerror("Communication Error",
               "UDP packet from QSpy too short")
            QView._quit(-2)
            return

        recID = packet[1]
        if recID == QSpy._PKT_ATTACH_CONF:
            QSpy._is_attached = True
            if QView._attach_dialog is not None:
                QView._attach_dialog.close()

            # send either reset or target-info request
            # (keep the poll0 loop running)
            if QView._reset_request:
                QView._reset_request = False
                QSpy._sendTo(pack("<B", QSpy._TRGT_RESET))
            else:
                QSpy._sendTo(pack("<B", QSpy._TRGT_INFO))

            # switch to the regular polling...
            QSpy._after_id = QView._gui.after(QSpy._POLLI, QSpy._poll)

            # only show the canvas, if visible
            QView._onCanvasView()
            return

        elif recID == QSpy._PKT_DETACH:
            QView._quit()
            return


    # regullar poll of the UDP socket after it has attached.
    @staticmethod
    def _poll():
        while True:
            try:
                packet = QSpy._sock.recv(4096)
                if not packet:
                    QView._showerror("UDP Socket Error",
                                     "Connection closed by QSpy")
                    QView._quit(-1)
                    return
            except OSError: # non-blocking socket...
                QSpy._after_id = QView._gui.after(QSpy._POLLI, QSpy._poll)
                return # <============= no packet at this time
            except:
                QView._showerror("UDP Socket Error",
                                 "Uknown UDP socket error")
                QView._quit(-1)
                return

            # parse the packet...
            dlen = len(packet)
            if dlen < 2:
                QView._showerror("UDP Socket Data Error",
                                 "UDP packet from QSpy too short")
                QView._quit(-2)
                return

            recID = packet[1]
            if recID == QSpy._PKT_TEXT_ECHO:
                # no need to check QView._echo_text.get()
                # because the text channel is closed
                QView.print_text(packet[3:])

            elif recID == QSpy._PKT_TARGET_INFO:
                if dlen != 18:
                    QView._showerror("UDP Socket Data Error",
                                     "Corrupted Target-info")
                    QView._quit(-2)
                    return

                if packet[4] & 0x80 != 0: # big endian?
                    QSpy._fmt_endian = ">"

                tstamp = packet[5:18]
                QSpy._size_objPtr  = tstamp[3] & 0x0F
                QSpy._size_funPtr  = tstamp[3] >> 4
                QSpy._size_tstamp  = tstamp[4] & 0x0F
                QSpy._size_sig     = tstamp[0] & 0x0F
                QSpy._size_evtSize = tstamp[0] >> 4
                QSpy._size_queueCtr= tstamp[1] & 0x0F
                QSpy._size_poolCtr = tstamp[2] >> 4
                QSpy._size_poolBlk = tstamp[2] & 0x0F
                QSpy._size_tevtCtr = tstamp[1] >> 4
                QSpy._fmt_target = "%02d%02d%02d_%02d%02d%02d"\
                    %(tstamp[12], tstamp[11], tstamp[10],
                      tstamp[9], tstamp[8], tstamp[7])
                #print("******* Target:", QSpy._fmt_target)
                QView._target.configure(text="Target: " + QSpy._fmt_target)
                QView._have_info = True

                # is this also target reset?
                if packet[2] != 0:
                    QView._onReset()
                    handler = getattr(QView._cust,
                                      "on_reset", None)
                    if handler is not None:
                        try:
                            handler() # call the packet handler
                        except:
                            QView._showerror("Runtime Error",
                                             traceback.format_exc(3))
                            QView._quit(-3)
                            return

            elif recID == QSpy._PKT_QF_RUN:
                handler = getattr(QView._cust,
                                  "on_run", None)
                if handler is not None:
                    try:
                        handler() # call the packet handler
                    except:
                        QView._showerror("Runtime Error",
                                         traceback.format_exc(3))
                        QView._quit(-3)
                        return

            elif recID == QSpy._PKT_DETACH:
                QView._quit()
                return

            elif recID <= 124: # other binary data
                # find the (global) handler for the packet
                handler = getattr(QView._cust,
                                  QSpy._QS[recID], None)
                if handler is not None:
                    try:
                        handler(packet) # call the packet handler
                    except:
                        QView._showerror("Runtime Error",
                                         traceback.format_exc(3))
                        QView._quit(-3)
                        return
            QSpy._rx_seq += 1
            QView._rx.configure(text="%d"%(QSpy._rx_seq))


    @staticmethod
    def _sendTo(packet, str=None):
        tx_packet = bytearray([QSpy._tx_seq & 0xFF])
        tx_packet.extend(packet)
        if str is not None:
            tx_packet.extend(bytes(str, "utf-8"))
            tx_packet.extend(b"\0") # zero-terminate
        try:
            QSpy._sock.sendto(tx_packet, QSpy._host_addr)
        except:
            QView._showerror("UDP Socket Error",
                                 traceback.format_exc(3))
            QView._quit(-1)
        QSpy._tx_seq += 1
        if not QView._gui is None:
            QView._tx.configure(text="%d"%(QSpy._tx_seq))

    @staticmethod
    def _sendEvt(ao_prio, signal, params = None):
        #print("evt:", signal, params)
        fmt = "<BB" + QSpy._fmt[QSpy._size_sig] + "H"
        if params is not None:
            length = len(params)
        else:
            length = 0

        if isinstance(signal, int):
            packet = bytearray(pack(
                fmt, QSpy._TRGT_EVENT, ao_prio, signal, length))
            if params is not None:
                packet.extend(params)
            QSpy._sendTo(packet)
        else:
            packet = bytearray(pack(
                fmt, QSpy._QSPY_SEND_EVENT, ao_prio, 0, length))
            if params is not None:
                packet.extend(params)
            QSpy._sendTo(packet, signal)


#=============================================================================
# DSL for QView customizations

# kinds of objects for current_obj()...
OBJ_SM = 0
OBJ_AO = 1
OBJ_MP = 2
OBJ_EQ = 3
OBJ_TE = 4
OBJ_AP = 5

# global filter groups...
GRP_ALL= 0xF0
GRP_SM = 0xF1
GRP_AO = 0xF2
GRP_MP = 0xF3
GRP_EQ = 0xF4
GRP_TE = 0xF5
GRP_QF = 0xF6
GRP_SC = 0xF7
GRP_SEM= 0xF8
GRP_MTX= 0xF9
GRP_U0 = 0xFA
GRP_U1 = 0xFB
GRP_U2 = 0xFC
GRP_U3 = 0xFD
GRP_U4 = 0xFE
GRP_UA = 0xFF
GRP_ON = GRP_ALL
GRP_OFF= -GRP_ALL

# local filter groups...
IDS_ALL= 0xF0
IDS_AO = (0x80 + 0)
IDS_EP = (0x80 + 64)
IDS_EQ = (0x80 + 80)
IDS_AP = (0x80 + 96)

HOME_DIR = None   ##< home directory of the customization script

## @brief Send the RESET packet to the Target
def reset_target(*args):
    if QView._have_info:
        QSpy._sendTo(pack("<B", QSpy._TRGT_RESET))
    else:
        QView._reset_request = True

## @brief executes a given command in the Target
# @sa qutest_dsl.command()
def command(cmdId, param1 = 0, param2 = 0, param3 = 0):
    if isinstance(cmdId, int):
        QSpy._sendTo(pack("<BBIII", QSpy._TRGT_COMMAND,
                         cmdId, param1, param2, param3))
    else:
        QSpy._sendTo(pack("<BBIII", QSpy._QSPY_SEND_COMMAND,
                         0, param1, param2, param3),
            cmdId) # add string command ID to end

## @brief trigger system clock tick in the Target
# @sa qutest_dsl.tick()
def tick(tick_rate = 0):
    QSpy._sendTo(pack("<BB", QSpy._TRGT_TICK, tick_rate))

## @brief peeks data in the Target
# @sa qutest_dsl.peek()
def peek(offset, size, num):
    QSpy._sendTo(pack("<BHBB", QSpy._TRGT_PEEK, offset, size, num))

## @brief pokes data into the Target
# @sa qutest_dsl.poke()
def poke(offset, size, data):
    fmt = "<BHBB" + ("x","B","H","x","I")[size]
    QSpy._sendTo(pack(fmt, QSpy._TRGT_POKE, offset, size, 1, data))

## @brief Set/clear the Global-Filter in the Target.
# @sa qutest_dsl.glb_filter()
def glb_filter(*args):
    # internal helper function
    def _apply(mask, is_neg):
        if is_neg:
            QView._glb_filter &= ~mask
        else:
            QView._glb_filter |= mask

    QView._glb_filter = 0
    for arg in args:
        # NOTE: positive filter argument means 'add' (allow),
        # negative filter argument meand 'remove' (disallow)
        is_neg = False
        if isinstance(arg, str):
            is_neg = (arg[0] == '-') # is  request?
            if is_neg:
                arg = arg[1:]
            try:
                arg = QSpy._QS.index(arg)
            except:
                QView._MessageDialog("Error in glb_filter()",
                                     'arg="' + arg + '"\n' +
                                     traceback.format_exc(3))
                sys.exit(-5) # return: event-loop might not be running yet
        else:
            is_neg = (arg < 0)
            if is_neg:
                arg = -arg

        if arg < 0x7F:
            _apply(1 << arg, is_neg)
        elif arg == GRP_ON:
            _apply(QSpy._GLB_FLT_MASK_ALL, is_neg)
        elif arg == GRP_SM:
            _apply(QSpy._GLB_FLT_MASK_SM, is_neg)
        elif arg == GRP_AO:
            _apply(QSpy._GLB_FLT_MASK_AO, is_neg)
        elif arg == GRP_MP:
            _apply(QSpy._GLB_FLT_MASK_MP, is_neg)
        elif arg == GRP_EQ:
            _apply(QSpy._GLB_FLT_MASK_EQ, is_neg)
        elif arg == GRP_TE:
            _apply(QSpy._GLB_FLT_MASK_TE, is_neg)
        elif arg == GRP_QF:
            _apply(QSpy._GLB_FLT_MASK_QF, is_neg)
        elif arg == GRP_SC:
            _apply(QSpy._GLB_FLT_MASK_SC, is_neg)
        elif arg == GRP_SEM:
            _apply(QSpy._GLB_FLT_MASK_SEM, is_neg)
        elif arg == GRP_MTX:
            _apply(QSpy._GLB_FLT_MASK_MTX, is_neg)
        elif arg == GRP_U0:
            _apply(QSpy._GLB_FLT_MASK_U0, is_neg)
        elif arg == GRP_U1:
            _apply(QSpy._GLB_FLT_MASK_U1, is_neg)
        elif arg == GRP_U2:
            _apply(QSpy._GLB_FLT_MASK_U2, is_neg)
        elif arg == GRP_U3:
            _apply(QSpy._GLB_FLT_MASK_U3, is_neg)
        elif arg == GRP_U4:
            _apply(QSpy._GLB_FLT_MASK_U4, is_neg)
        elif arg == GRP_UA:
            _apply(QSpy._GLB_FLT_MASK_UA, is_neg)
        else:
            assert 0, "invalid global filter arg=0x%X"%(arg)

    QSpy._sendTo(pack("<BBQQ", QSpy._TRGT_GLB_FILTER, 16,
                      QView._glb_filter & 0xFFFFFFFFFFFFFFFF,
                      QView._glb_filter >> 64))
    QView._updateMenus()

## @brief Set/clear the Local-Filter in the Target.
# @sa qutest_dsl.loc_filter()
def loc_filter(*args):
    # internal helper function
    def _apply(mask, is_neg):
        if is_neg:
            QView._loc_filter &= ~mask
        else:
            QView._loc_filter |= mask

    for arg in args:
        # NOTE: positive filter argument means 'add' (allow),
        # negative filter argument means 'remove' (disallow)
        is_neg = (arg < 0)
        if is_neg:
            arg = -arg

        if arg < 0x7F:
            _apply(1 << arg, is_neg)
        elif arg == IDS_ALL:
            _apply(QSpy._LOC_FLT_MASK_ALL, is_neg)
        elif arg == IDS_AO:
            _apply(QSpy._LOC_FLT_MASK_AO, is_neg)
        elif arg == IDS_EP:
            _apply(QSpy._LOC_FLT_MASK_EP, is_neg)
        elif arg == IDS_EQ:
            _apply(QSpy._LOC_FLT_MASK_EQ, is_neg)
        elif arg == IDS_AP:
            _apply(QSpy._LOC_FLT_MASK_AP, is_neg)
        else:
            assert 0, "invalid local filter arg=0x%X"%(arg)

    QSpy._sendTo(pack("<BBQQ", QSpy._TRGT_LOC_FILTER, 16,
                      QView._loc_filter & 0xFFFFFFFFFFFFFFFF,
                      QView._loc_filter >> 64))

## @brief Set/clear the Active-Object Local-Filter in the Target.
# @sa qutest_dsl.ao_filter()
def ao_filter(obj_id):
    # NOTE: positive obj_id argument means 'add' (allow),
    # negative obj_id argument means 'remove' (disallow)
    remove = 0
    QView._locAO_OBJ.set(obj_id)
    QView._menu_loc_filter.entryconfig("AO-OBJ...",
            accelerator=QView._locAO_OBJ.get())
    if isinstance(obj_id, str):
        if obj_id[0:1] == '-': # is it remvoe request?
            obj_id = obj_id[1:]
            remove = 1
        QSpy._sendTo(pack("<BB" + QSpy._fmt[QSpy._size_objPtr],
            QSpy._QSPY_SEND_AO_FILTER, remove, 0),
            obj_id) # add string object-ID to end
    else:
        if obj_id < 0:
            obj_id = -obj_id
            remove = 1
        QSpy._sendTo(pack("<BB" + QSpy._fmt[QSpy._size_objPtr],
            QSpy._TRGT_AO_FILTER, remove, obj_id))

## @brief Set the Current-Object in the Target.
# @sa qutest_dsl.current_obj()
def current_obj(obj_kind, obj_id):
    if obj_id == "":
        return
    if isinstance(obj_id, int):
        QSpy._sendTo(pack("<BB" + QSpy._fmt[QSpy._size_objPtr],
            QSpy._TRGT_CURR_OBJ, obj_kind, obj_id))
        obj_id = "0x%08X"%(obj_id)
    else:
        QSpy._sendTo(pack("<BB" + QSpy._fmt[QSpy._size_objPtr],
            QSpy._QSPY_SEND_CURR_OBJ, obj_kind, 0), obj_id)

    QView._currObj[obj_kind].set(obj_id)
    QView._menu_curr_obj.entryconfig(obj_kind, accelerator=obj_id)
    QView._updateMenus()

## @brief query the @ref current_obj() "current object" in the Target
# @sa qutest_dsl.query_curr()
def query_curr(obj_kind):
    QSpy._sendTo(pack("<BB", QSpy._TRGT_QUERY_CURR, obj_kind))

## @brief publish a given event to subscribers in the Target
# @sa qutest_dsl.publish()
def publish(signal, params = None):
    QSpy._sendEvt(QSpy._EVT_PUBLISH, signal, params)

## @brief post a given event to the current AO object in the Target
# @sa qutest_dsl.post()
def post(signal, params = None):
    QSpy._sendEvt(QSpy._EVT_POST, signal, params)

## @brief take the top-most initial transition in the
# current SM object in the Target
# @sa qutest_dsl.init()
def init(signal = 0, params = None):
    QSpy._sendEvt(QSpy._EVT_INIT, signal, params)

## @brief dispatch a given event in the current SM object in the Target
# @sa qutest_dsl.dispatch()
def dispatch(signal, params = None):
    QSpy._sendEvt(QSpy._EVT_DISPATCH, signal, params)

## @brief Unpack a QS trace record
#
# @description
# The qunpack() facility is similar to Python `struct.unpack()`,
# specifically designed for unpacking binary QP/Spy packets.
# qunpack() handles all data formats supported by struct.unpack()`,
# plus data formats specific to QP/Spy. The main benefit of qunpack()
# is that it automatically applies the Target-supplied info about
# various the sizes of various elements, such as Target timestamp,
# Target object-pointer, Target event-signal, zero-terminated string, etc.
## @brief pokes data into the Target
# @sa qutest_dsl.poke()
#
# @param[in] fmt  format string
# @param[in] bstr byte-string to unpack
#
# @returns
# The result is a tuple with elements corresponding to the format items.
#
# The additional format characters have the following meaning:
#
# - T : QP/Spy timestamp -> integer, 2..4-bytes (Target dependent)
# - O : QP/Spy object pointer -> integer, 2..8-bytes (Target dependent)
# - F : QP/Spy function pointer -> integer, 2..8-bytes (Target dependent)
# - S : QP/Spy event signal -> integer, 1..4-bytes (Target dependent)
# - Z : QP/Spy zero-terminated string -> string of n-bytes (variable length)
#
# @usage
# @include qunpack.py
#
def qunpack(fmt, bstr):
    n = 0
    m = len(fmt)
    bord = "<" # default little-endian byte order
    if fmt[0:1] in ("@", "=", "<", ">", "!"):
        bord = fmt[0:1]
        n += 1
    data = []
    offset = 0
    while n < m:
       fmt1 = fmt[n:(n+1)]
       u = ()
       if fmt1 in ("B", "b", "c", "x", "?"):
           u = struct.unpack_from(bord + fmt1, bstr, offset)
           offset += 1
       elif fmt1 in ("H", "h"):
           u = struct.unpack_from(bord + fmt1, bstr, offset)
           offset += 2
       elif fmt1 in ("I", "L", "i", "l", "f"):
           u = struct.unpack_from(bord + fmt1, bstr, offset)
           offset += 4
       elif fmt1 in ("Q", "q", "d"):
           u = struct.unpack_from(bord + fmt1, bstr, offset)
           offset += 8
       elif fmt1 == "T":
           u = struct.unpack_from(bord + QSpy._fmt[QSpy._size_tstamp],
                                  bstr, offset)
           offset += QSpy._size_tstamp
       elif fmt1 == "O":
           u = struct.unpack_from(bord + QSpy._fmt[QSpy._size_objPtr],
                                  bstr, offset)
           offset += QSpy._size_objPtr
       elif fmt1 == "F":
           u = struct.unpack_from(bord + QSpy._fmt[QSpy._size_funPtr],
                                  bstr, offset)
           offset += QSpy._size_funPtr
       elif fmt1 == "S":
           u = struct.unpack_from(bord + QSpy._fmt[QSpy._size_sig],
                                  bstr, offset)
           offset += QSpy._size_sig
       elif fmt1 == "Z": # zero-terminated C-string
           end = offset
           while bstr[end]: # not zero-terminator?
               end += 1
           u = (bstr[offset:end].decode(),)
           offset = end + 1 # inclue the terminating zero
       else:
           assert 0, "qunpack(): unknown format"
       data.extend(u)
       n += 1
    return tuple(data)

#=============================================================================
# main entry point to QView
def main():
    # process command-line arguments...
    argv = sys.argv
    argc = len(argv)
    arg  = 1 # skip the "qview" argument

    if "-h" in argv or "--help" in argv or "?" in argv:
        print("\nUsage: python qview.pyw [custom-script] "
            "[qspy_host[:udp_port]] [local_port]\n\n"
            "help at: https://www.state-machine.com/qtools/QView.html")
        sys.exit(0)

    script = ""
    if arg < argc:
        # is the next argument a test script?
        if argv[arg].endswith(".py") or argv[arg].endswith(".pyw"):
            script = argv[arg]
            arg += 1

        if arg < argc:
            host_port = argv[arg].split(":")
            arg += 1
            if len(host_port) > 0:
                QSpy._host_addr[0] = host_port[0]
            if len(host_port) > 1:
                QSpy._host_addr[1] = int(host_port[1])

        if arg < argc:
            QSpy._local_port = int(argv[arg])

    QSpy._host_addr = tuple(QSpy._host_addr) # convert to immutable tuple
    #print("Connection: ", QSpy._host_addr, QSpy._local_port)

    # create the QView GUI
    QView._gui = Tk()
    QView._gui.title("QView " + \
        "%d.%d.%d"%(QView.VERSION//100, \
                   (QView.VERSION//10) % 10, \
                    QView.VERSION % 10) + " -- " + script)
    QView._init_gui(QView._gui)

    # extend the QView with a custom sript
    if script != "":
        try:
            # set the (global) home directory of the custom script
            global HOME_DIR
            HOME_DIR = os.path.dirname(os.path.realpath(script))

            with open(script) as f:
                code = compile(f.read(), script, "exec")

            exec(code) # execute the script
        except: # error opening the file or error in the script
            # NOTE: don't use QView._showError() because the
            # polling loop is not running yet.
            QView._MessageDialog("Error in " + script,
                                 traceback.format_exc(3))
            sys.exit(-4) # return: event-loop is not running yet

    err = QSpy._init()
    if err:
        sys.exit(err) # simple return: event-loop is not running yet

    QSpy._attach()
    QView._gui.mainloop()
    QView._gui  = None
    QSpy._detach()

    sys.exit(QView._err)

#=============================================================================
if __name__ == "__main__":
    main()

