
/*******************************************************************************/
/* Copyright (C) 2008-2021 Jonathan Moore Liles (as "Non-Mixer")               */
/* Copyright (C) 2021- Stazed                                                  */
/*                                                                             */
/* This file is part of Non-Mixer-XT                                           */
/*                                                                             */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

#pragma once

#include <FL/Fl.H>
#include "DPM.H"
#include "Panner.H"

#include "../../FL/Fl_Scalepack.H"
#include "../../FL/Fl_Flip_Button.H"
#include "../../FL/Fl_Sometimes_Input.H"
#include "../../nonlib/JACK/Port.H"
#include "../../nonlib/Loggable.H"

#include <FL/Fl_Dial.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Counter.H>
#include "NMXT_Progress.H"


class Chain;
class Controller_Module;
class Meter_Indicator_Module;
class Module;
class Fl_Flip_Button;
class Fl_Input;
class Fl_Menu_;
class Fl_Menu_Button;
class Fl_Choice;
class Group;

#include "Module.H"

class Mixer_Strip : public Fl_Group, public Loggable
{

public:

    static int min_h ( void )
    {
        return 333;
    }

    explicit Mixer_Strip( const char *strip_name );
    Mixer_Strip();                                              /* for log create */
    virtual ~Mixer_Strip();

    void chain ( Chain *c );
    Chain *chain ( void )
    {
        return _chain;
    }

    virtual void log_children ( void ) const override;

    virtual void color ( Fl_Color c );
    virtual Fl_Color color ( void ) const;

    LOG_CREATE_FUNC( Mixer_Strip );

    static void configure_outputs ( Fl_Widget *o, void *v );
    void configure_outputs ( void );

    bool configure_ports ( int n );

    void handle_module_added ( Module *m );
    void handle_module_removed ( Module *m );

    void update ( void );

    void name ( const char *name );
    const char *name ( void ) const
    {
        return label();
    }

private:

    char *_auto_input;
    void auto_input ( const char *s );

    unsigned int _dsp_load_index;

    /* used to defer setting the mode of the gain controller until the
     chain has been added and the controller connected to a default
     module */
    int _gain_controller_mode;
    int _mute_controller_mode;
    bool _manual_connection;
    int _number;

    Fl_Menu_Button *output_connection_button;
    Fl_Flip_Button *width_button;
    Fl_Flip_Button *tab_button;
    Fl_Button *close_button;
    Fl_Input *name_field;
    Fl_Choice *group_choice;

    Fl_Group *tab_group;
    Fl_Group *signal_tab;
    Fl_Group *fader_tab;

    Chain *_chain;
    Group *_group;

    Fl_Box *spatialization_label;
    Controller_Module *gain_controller;
    Controller_Module *mute_controller;
    Controller_Module *jack_input_controller;
    Controller_Module *spatialization_controller;
    Meter_Indicator_Module *meter_indicator;

    NMXT_Progress *dsp_load_progress;

    Fl_Box *color_box;

    Fl_Color _color;

    void init ( );
    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    void set_tab ( void );

    void update_port_names ( void );

    void menu_cb ( const Fl_Menu_ *m );
    static void menu_cb ( Fl_Widget *w, void *v );
    Fl_Menu_Button & menu ( void ) const;

    static void snapshot ( void *v );
    void snapshot ( void );
    bool export_strip ( const char *filename );

    void set_spatializer_visibility ( void );

protected:

    void get ( Log_Entry &e ) const override;
    void set ( Log_Entry &e ) override;

    virtual int handle ( int m ) override;
    virtual void draw ( void ) override;

public:

    void manual_connection ( bool b );
    bool has_group_affinity ( void ) const;
    void disconnect_auto_inputs ( const char *exclude_pattern );
    void auto_connect_outputs ( void );
    bool maybe_auto_connect_output ( Module::Port *p );

    void get_output_ports ( std::list<std::string> &ports );

    void update_group_choice ( void );

    Controller_Module *spatializer ( void );

    Group *group ( void )
    {
        return _group;
    }
    void clear_group ( void )
    {
        _group = NULL;
    }

    //  int group ( void ) const;
    void group ( Group * );
    void send_feedback ( bool force );
    void schedule_feedback ( void );
    int number ( void ) const;
    void number ( int );
    static bool import_strip ( const char *filename );

    void command_toggle_fader_view ( void );
    void command_move_left ( void );
    void command_move_right ( void );
    void command_close ( void );
    void command_rename ( const char * s );
    void command_width ( bool b );
    void command_view ( bool b );

};
