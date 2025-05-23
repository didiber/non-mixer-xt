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

#include "const.h"

#include "Controller_Module.H"

#include <stdio.h>

#include "../../FL/menu_popup.H"
#include "../../FL/test_press.H"
#include "../../FL/Fl_Labelpad_Group.H"
#include "../../FL/Fl_Value_SliderX.H"
#include "../../nonlib/OSC/Endpoint.H"
#include "../../nonlib/string_util.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Light_Button.H>
#include <FL/fl_draw.H>
#include "Panner.H"

#include "Chain.H"

// needed for mixer->endpoint
#include "Mixer.H"
#include "Spatialization_Console.H"

bool Controller_Module::learn_by_number = false;
bool Controller_Module::_learn_mode = false;

Controller_Module* Controller_Module::_learning_control = NULL;

void
Controller_Module::take_focus( void )
{
    bool v = visible_focus ( );

    if ( !v )
        set_visible_focus ( );

    Fl_Widget::take_focus ( );

    if ( !v )
        clear_visible_focus ( );
}

Controller_Module::Controller_Module( bool is_default ) :
    Module( is_default, 50, 100, name( ) ),
    _horizontal( true ),
    _pad( true ),
    control_value( 0.0f ),
    _mode( GUI ),
    control( 0 )
{
    box ( FL_NO_BOX );

    Module::add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );

    //    mode( GUI );
    //    mode( CV );
    //    configure_inputs( 1 );

    end ( );
    log_create ( );
}

Controller_Module::~Controller_Module( )
{
    log_destroy ( );

    /* shutdown JACK port, if we have one */
    mode ( GUI );
}

void
Controller_Module::handle_chain_name_changed( )
{
    if ( type ( ) == SPATIALIZATION )
    {
        if ( Mixer::spatialization_console )
            Mixer::spatialization_console->update ( );
    }

    //    change_osc_path( generate_osc_path() );
}

void
Controller_Module::handle_control_disconnect( Module::Port * /*p*/ )
{
    if ( type ( ) == SPATIALIZATION )
    {
        if ( Mixer::spatialization_console )
            Mixer::spatialization_console->update ( );
    }
}

void
Controller_Module::disconnect( void )
{
    for ( unsigned int i = 0; i < control_output.size ( ); ++i )
        control_output[i].disconnect ( );
}

void
Controller_Module::get( Log_Entry &e ) const
{
    Module::get ( e );

    Port *p = control_output[0].connected_port ( );

    if ( !p )
    {
        e.add ( ":module", "" );
        e.add ( ":port", "" );
        e.add ( ":mode", "" );
    }
    else
    {
        Module *m = p->module ( );

        e.add ( ":module", m );
        e.add ( ":port", m->control_input_port_index ( p ) );
        e.add ( ":mode", mode ( ) );
    }
}

void
Controller_Module::set( Log_Entry &e )
{
    Module::set ( e );

    int port = -1;
    Module *module = NULL;

    for ( int i = 0; i < e.size ( ); ++i )
    {
        const char *s, *v;

        e.get ( i, &s, &v );

        if ( !strcmp ( s, ":port" ) )
        {
            port = atoi ( v );
        }
        else if ( !strcmp ( s, ":module" ) )
        {
            unsigned int ii;
            sscanf ( v, "%X", &ii );
            Module *t = static_cast<Module*> ( Loggable::find ( ii ) );

            assert ( t );

            module = t;
        }
    }

    if ( port >= 0 && module )
    {
        connect_to ( &module->control_input[port] );
        module->chain ( )->add_control ( this );
    }

    for ( int i = 0; i < e.size ( ); ++i )
    {
        const char *s, *v;

        e.get ( i, &s, &v );

        if ( !strcmp ( s, ":mode" ) )
        {
            mode ( (Mode) atoi ( v ) );
        }
    }

}

void
Controller_Module::mode( Mode m )
{
    if ( mode ( ) != CV && m == CV )
    {
        if ( control_output[0].connected ( ) )
        {
            chain ( )->client ( )->lock ( );

            Port *p = control_output[0].connected_port ( );

            char prefix[512];

            const Module *pm = p->module ( );

            if ( pm->number ( ) >= 0 )
                /* we do it this way now to ensure uniqueness */
                snprintf ( prefix, sizeof (prefix ), "CV-%s/%s", pm->label ( ), p->name ( ) );
            else
                snprintf ( prefix, sizeof (prefix ), "CV-%s", p->name ( ) );

            add_aux_cv_input ( prefix, 0 );

            chain ( )->client ( )->unlock ( );
        }
    }
    else if ( mode ( ) == CV && m != CV )
    {
        chain ( )->client ( )->lock ( );

        aux_audio_input.back ( ).jack_port ( )->shutdown ( );

        delete aux_audio_input.back ( ).jack_port ( );

        aux_audio_input.pop_back ( );

        chain ( )->client ( )->unlock ( );
    }

    _mode = m;
}

bool
Controller_Module::connect_spatializer_radius_to( Module *m )
{
    Port *radius_port = NULL;
    float radius_value = 0.0f;

    for ( unsigned int i = 0; i < m->control_input.size ( ); ++i )
    {
        Port *p = &m->control_input[i];

        if ( !strcasecmp ( "Radius", p->name ( ) ) )
            /* 90.0f == p->hints.maximum && */
            /* -90.0f == p->hints.minimum ) */
        {
            radius_port = p;
            radius_value = p->control_value ( );
            continue;
        }
    }

    if ( !radius_port )
        return false;

    if ( control_output.size ( ) != 3 )
    {
        control_output.clear ( );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
    }

    control_output[2].connect_to ( radius_port );

    maybe_create_panner ( );

    Panner *o = static_cast<Panner*> ( control );

    o->point ( 0 )->radius ( radius_value );

    if ( Mixer::spatialization_console )
        Mixer::spatialization_console->update ( );

    return true;
}

void
Controller_Module::maybe_create_panner( void )
{
    if ( _type != SPATIALIZATION )
    {
        clear ( );

        Panner *o = new Panner ( 0, 0, 92, 92 );

        o->box ( FL_FLAT_BOX );
        o->color ( FL_GRAY0 );
        o->selection_color ( FL_BACKGROUND_COLOR );
        o->labeltype ( FL_NORMAL_LABEL );
        o->labelfont ( 0 );
        o->labelcolor ( FL_FOREGROUND_COLOR );
        o->align ( FL_ALIGN_TOP );
        o->when ( FL_WHEN_CHANGED );
        label ( "Spatialization" );

        o->align ( FL_ALIGN_TOP );
        o->labelsize ( 10 );
        //        o->callback( cb_panner_value_handle, new callback_data( this, azimuth_port_number, elevation_port_number ) );

        o->callback ( cb_spatializer_handle, this );

        control = static_cast<Panner*>( o );

        if ( _pad )
        {
            Fl_Labelpad_Group *flg = new Fl_Labelpad_Group ( o );
            flg->position ( x ( ), y ( ) );
            flg->set_visible_focus ( );
            size ( flg->w ( ), flg->h ( ) );
            add ( flg );
        }
        else
        {
            //            o->clear_visible_focus();
            o->resize ( x ( ), y ( ), w ( ), h ( ) );
            add ( o );
            resizable ( o );
            init_sizes ( );
        }

        _type = SPATIALIZATION;
    }
}

/** attempt to transform this controller into a spatialization
    controller and connect to the given module's spatialization
    control inputs. Returns true on success, false if given module
    does not accept spatialization inputs. */
bool
Controller_Module::connect_spatializer_to( Module *m )
{
    connect_spatializer_radius_to ( m );

    /* these are for detecting related parameter groups which can be
       better represented by a single control */
    Port *azimuth_port = NULL;
    float azimuth_value = 0.0f;
    Port *elevation_port = NULL;
    float elevation_value = 0.0f;

    for ( unsigned int i = 0; i < m->control_input.size ( ); ++i )
    {
        Port *p = &m->control_input[i];

        if ( !strcasecmp ( "Azimuth", p->name ( ) ) &&
            180.0f == p->hints.maximum &&
            -180.0f == p->hints.minimum )
        {
            azimuth_port = p;
            azimuth_value = p->control_value ( );
            continue;
        }
        else if ( !strcasecmp ( "Elevation", p->name ( ) ) &&
            90.0f == p->hints.maximum &&
            -90.0f == p->hints.minimum )
        {
            elevation_port = p;
            elevation_value = p->control_value ( );
            continue;
        }
    }

    if ( !( azimuth_port && elevation_port ) )
        return false;

    if ( control_output.size ( ) != 3 )
    {
        control_output.clear ( );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
        add_port ( Port ( this, Port::OUTPUT, Port::CONTROL ) );
    }

    control_output[0].connect_to ( azimuth_port );
    control_output[1].connect_to ( elevation_port );

    maybe_create_panner ( );

    Panner *o = static_cast<Panner*> ( control );

    o->point ( 0 )->azimuth ( azimuth_value );
    o->point ( 0 )->elevation ( elevation_value );

    if ( Mixer::spatialization_console )
        Mixer::spatialization_console->update ( );

    return true;
}

void
Controller_Module::apply_label( Port *p, Fl_Widget *o )
{
    char path[256];

    if ( is_default ( ) )
        snprintf ( path, sizeof (path ) - 1, "%s", p->name ( ) );
    else
        snprintf ( path, sizeof (path ) - 1, "%s/%s", p->module ( )->label ( ), p->name ( ) );

    o->copy_label ( path );
}

void
Controller_Module::connect_to( Port *p )
{
    control_output[0].connect_to ( p );

#ifdef LV2_SUPPORT
    /* The controller needs the ScalePoints vector to set the Fl_Choice menu based on
       ScalePoints Value since the Fl_Menu value may differ from the controller value */
    control_output[0].hints.ScalePoints = p->hints.ScalePoints;
#endif
    clear ( );

    Fl_Widget *w;

    if ( p->hints.type == Module::Port::Hints::BOOLEAN )
    {
        Fl_Button *o = new Fl_Button ( 0, 0, 200, 20 );
        w = o;
        o->type ( FL_TOGGLE_BUTTON );
        o->value ( p->control_value ( ) );
        o->selection_color ( fl_color_average ( FL_GRAY, FL_CYAN, 0.5 ) );

        _type = TOGGLE;

        control = static_cast<Fl_Button*> ( o );
    }
    else if ( p->hints.type == Module::Port::Hints::INTEGER )
    {

        Fl_Counter *o = new Fl_Counter ( 0, 0, 200, 20 );

        control = o;
        w = o;

        o->type ( 1 );
        o->step ( 1 );

        if ( p->hints.ranged )
        {
            o->minimum ( p->hints.minimum );
            o->maximum ( p->hints.maximum );
        }

        _type = SPINNER;

        o->value ( p->control_value ( ) );
    }
#ifdef LV2_SUPPORT
    else if ( p->hints.type == Module::Port::Hints::LV2_INTEGER_ENUMERATION )
    {
        Fl_Choice *o = new Fl_Choice ( 0, 0, 200, 20 );
        o->selection_color ( fl_color_average ( FL_GRAY, FL_CYAN, 0.5 ) );

        /* Add the choice labels */
        for ( unsigned count = 0; count < p->hints.ScalePoints.size ( ); ++count )
        {
            o->add ( p->hints.ScalePoints[count].Label.c_str ( ) );
        }

        control = static_cast<Fl_Choice*> ( o );
        w = o;

        _type = CHOICE;

        /* We set the Fl_Choice menu according to the position in the ScalePoints vector */
        int menu_location = 0;

        for ( unsigned i = 0; i < p->hints.ScalePoints.size ( ); ++i )
        {
            if ( (int) p->hints.ScalePoints[i].Value == (int) ( p->control_value ( ) + .5 ) ) // .5 for float rounding
            {
                menu_location = i;
                break;
            }
        }

        o->value ( menu_location );
    }
#endif  // LV2_SUPPORT
    //  else if ( p->hints.type == Module::Port::Hints::LOGARITHMIC || Module::Port::Hints::LV2_INTEGER )
    else
    {
        Fl_Value_SliderX *o = new Fl_Value_SliderX ( 0, 0, 30, 250 );

        control = static_cast<Fl_Value_SliderX*> ( o );
        w = o;

        if ( !_horizontal )
        {
            o->size ( 30, 250 );
            o->type ( FL_VERT_NICE_SLIDER );
        }
        else
        {
            o->size ( 200, 20 );
            o->type ( FL_HOR_NICE_SLIDER );
        }

        //        o->type(4);
        o->color ( FL_BACKGROUND2_COLOR );
        o->selection_color ( fl_color_average ( FL_GRAY, FL_CYAN, 0.5 ) );
        o->minimum ( 1.5 );
        o->maximum ( 0 );
        o->value ( 1 );
        //        o->textsize(9);

        if ( p->hints.ranged )
        {
            if ( !_horizontal )
            {
                o->minimum ( p->hints.maximum );
                o->maximum ( p->hints.minimum );
            }
            else
            {
                o->minimum ( p->hints.minimum );
                o->maximum ( p->hints.maximum );
            }
        }
#if defined(LV2_SUPPORT) || defined(VST2_SUPPORT)
        if ( p->hints.type == Module::Port::Hints::LV2_INTEGER )
        {
            o->precision ( 0 );
        }
        else
#endif
        {
            o->precision ( 2 );
        }

        o->value ( p->control_value ( ) );

        _type = SLIDER;
    }
    /* else */
    /* { */
    /*     { Fl_DialX *o = new Fl_DialX( 0, 0, 50, 50, p->name() ); */
    /*         w = o; */
    /*         control = o; */

    /*         if ( p->hints.ranged ) */
    /*         { */
    /*             DMESSAGE( "Min: %f, max: %f", p->hints.minimum, p->hints.maximum ); */
    /*             o->minimum( p->hints.minimum ); */
    /*             o->maximum( p->hints.maximum ); */
    /*         } */

    /*         o->color( fl_darker( FL_GRAY ) ); */
    /*         o->selection_color( FL_WHITE ); */
    /*         o->value( p->control_value() ); */
    /*     } */

    /*     _type = KNOB; */
    /* } */

    apply_label ( p, control );

    control_value = p->control_value ( );

    w->clear_visible_focus ( );
    w->align ( FL_ALIGN_TOP );
    w->labelsize ( 10 );
    w->callback ( cb_handle, this );

    if ( _pad )
    {
        Fl_Labelpad_Group *flg = new Fl_Labelpad_Group ( w );
        flg->set_visible_focus ( );
        size ( flg->w ( ), flg->h ( ) );
        flg->position ( x ( ), y ( ) );
        add ( flg );
        resizable ( flg );
        //        init_sizes();
    }
    else
    {
        /* HACK: hide label */
        if ( _type == TOGGLE )
        {
            w->align ( FL_ALIGN_INSIDE );
        }
        else
        {
            w->labeltype ( FL_NO_LABEL );
        }
        w->resize ( x ( ), y ( ), this->w ( ), h ( ) );
        add ( w );
        resizable ( w );
        init_sizes ( );
    }
}

void
Controller_Module::update( void )
{
    /* we only need this in CV (JACK) mode, because with other forms
     * of control the change happens in the GUI thread and we know it */
    if ( mode ( ) != CV )
        return;

    /* ensures that port value change callbacks are run */
    if ( control && control_output.size ( ) > 0 && control_output[0].connected ( ) )
        control_output[0].connected_port ( )->control_value ( control_value );
}

void
Controller_Module::cb_handle( Fl_Widget *w, void *v )
{
    ( (Controller_Module*) v )->cb_handle ( w );
}

void
Controller_Module::cb_handle( Fl_Widget *w )
{
    if ( type ( ) == TOGGLE )
    {
        control_value = ( (Fl_Button*) w )->value ( );
    }
#ifdef LV2_SUPPORT
    else if ( type ( ) == CHOICE )
    {
        /* We set the control value according to the menu position in the ScalePoints vector */
        int menu_location = (int) ( (Fl_Choice*) w )->value ( );
        control_value = control_output[0].hints.ScalePoints[menu_location].Value;
    }
#endif
    else
        control_value = ( (Fl_Valuator*) w )->value ( );

    if ( control_output[0].connected ( ) )
        control_output[0].connected_port ( )->control_value ( control_value );
}

void
Controller_Module::cb_spatializer_handle( Fl_Widget *w, void *v )
{
    ( (Controller_Module*) v )->cb_spatializer_handle ( w );
}

void
Controller_Module::cb_spatializer_handle( Fl_Widget *w )
{
    Panner *pan = static_cast<Panner*> ( w );

    if ( control_output[0].connected ( ) &&
        control_output[1].connected ( ) )
    {
        control_output[0].connected_port ( )->control_value ( pan->point ( 0 )->azimuth ( ) );
        control_output[1].connected_port ( )->control_value ( pan->point ( 0 )->elevation ( ) );
    }

    if ( control_output[2].connected ( ) )
    {
        control_output[2].connected_port ( )->control_value ( pan->point ( 0 )->radius ( ) );
    }
}

void
Controller_Module::menu_cb( Fl_Widget *w, void *v )
{
    ( (Controller_Module*) v )->menu_cb ( (Fl_Menu_*) w );
}

void
Controller_Module::menu_cb( const Fl_Menu_ *m )
{
    char picked[256];

    m->item_pathname ( picked, sizeof ( picked ) );

    Logger log ( this );

    if ( !strcmp ( picked, "Mode/GUI + OSC" ) )
        mode ( GUI );
    else if ( !strcmp ( picked, "Mode/Control Voltage (JACK)" ) )
        mode ( CV );
    else if ( !strcmp ( picked, "/Remove" ) )
        command_remove ( );
    else if ( !strncmp ( picked, "Connect To/", strlen ( "Connect To/" ) ) )
    {
        char *peer_name = index ( picked, '/' ) + 1;

        *index ( peer_name, '/' ) = 0;

        //        OSC::Signal s = (OSC::Signal*)m->mvalue()->user_data();
        const char *path = ( ( OSC::Signal* )m->mvalue ( )->user_data ( ) )->path ( );

        /* if ( ! _osc_output()->is_connected_to( ((OSC::Signal*)m->mvalue()->user_data()) ) ) */
        /* { */
        /* _persistent_osc_connections.push_back( strdup(path) ); */

        Port *p = control_output[0].connected_port ( );

        if ( learn_by_number )
            mixer->osc_endpoint->add_translation ( path, p->osc_number_path ( ) );
        else
            mixer->osc_endpoint->add_translation ( path, p->osc_path ( ) );
    }
    else if ( !strncmp ( picked, "Disconnect From/", strlen ( "Disconnect From/" ) ) )
    {
        /* char *peer_name = index( picked, '/' ) + 1; */

        /* *index( peer_name, '/' ) = 0; */

        //        OSC::Signal s = (OSC::Signal*)m->mvalue()->user_data();
        const char *path = static_cast<const char*> ( m->mvalue ( )->user_data ( ) );

        /* if ( ! _osc_output()->is_connected_to( ((OSC::Signal*)m->mvalue()->user_data()) ) ) */
        /* { */
        /* _persistent_osc_connections.push_back( strdup(path) ); */

        //        Port *p = control_output[0].connected_port();

        mixer->osc_endpoint->del_translation ( path );

        /* if ( learn_by_number ) */
        /* { */
        /*     char *our_path = p->osc_number_path(); */

        /*     mixer->osc_endpoint->add_translation( path, our_path ); */

        /*     free(our_path); */
        /* } */
        /* else */
        /*     mixer->osc_endpoint->add_translation( path, p->osc_path() ); */
    }

    /* } */
    /* else */
    /* { */
    /*     /\* timeline->osc->disconnect_signal( _osc_output(), path ); *\/ */

    /*     /\* for ( std::list<char*>::iterator i = _persistent_osc_connections.begin(); *\/ */
    /*     /\*       i != _persistent_osc_connections.end(); *\/ */
    /*     /\*       ++i ) *\/ */
    /*     /\* { *\/ */
    /*     /\*     if ( !strcmp( *i, path ) ) *\/ */
    /*     /\*     { *\/ */
    /*     /\*         free( *i ); *\/ */
    /*     /\*         i = _persistent_osc_connections.erase( i ); *\/ */
    /*     /\*         break; *\/ */
    /*     /\*     } *\/ */
    /*     /\* } *\/ */

    /*     //free( path ); */
    /* } */

}
static Fl_Menu_Button *peer_menu;
static const char *peer_prefix;

void
Controller_Module::peer_callback( OSC::Signal *sig, OSC::Signal::State /*state*/, void *v )
{
    char *s;

    DMESSAGE ( "Paramter limits: %f %f", sig->parameter_limits ( ).min, sig->parameter_limits ( ).max );

    /* only show outputs */
    if ( sig->direction ( ) != OSC::Signal::Output )
        return;

    /* only list CV signals for now */
    if ( !( sig->parameter_limits ( ).min == 0.0 &&
        sig->parameter_limits ( ).max == 1.0 ) )
        return;

    if ( !v )
    {
        /* if( state == OSC::Signal::Created ) */
        /*     timeline->connect_osc(); */
        /* else */
        /*     timeline->update_osc_connection_state(); */
    }
    else
    {
        /* building menu */
        //        const char *name = sig->peer_name();

        assert ( sig->path ( ) );

        char *path = strdup ( sig->path ( ) );

        unescape_url ( path );

        if ( path == NULL )
            return;

        asprintf ( &s, "%s%s", peer_prefix, path );

        peer_menu->add ( s, 0, NULL, static_cast<void*> ( sig ), 0 );

        /*     FL_MENU_TOGGLE | */
        /* ( ((Controller_Module*)v)->_osc_output()->is_connected_to( sig ) ? FL_MENU_VALUE : 0 ) ); */

        free ( path );

        free ( s );
    }
}

void
Controller_Module::add_osc_peers_to_menu( Fl_Menu_Button *m, const char *prefix )
{
    mixer->osc_endpoint->peer_signal_notification_callback ( &Controller_Module::peer_callback, NULL );

    peer_menu = m;
    peer_prefix = prefix;

    mixer->osc_endpoint->list_peer_signals ( this );
}

void
Controller_Module::add_osc_connections_to_menu( Fl_Menu_Button *, const char *prefix )
{
    /* peer_menu = m; */
    const char *a_peer_prefix = prefix;

    //    mixer->osc_endpoint->list_peer_signals( this );

    Port *p = control_output[0].connected_port ( );

    const char *number_path = p->osc_number_path ( );
    const char *name_path = p->osc_path ( );

    const char *paths[] = { number_path, name_path, NULL };

    for ( const char **cpath = paths; *cpath; cpath++ )
    {
        const char ** conn = mixer->osc_endpoint->get_connections ( *cpath );

        if ( conn )
        {
            for ( const char **s = conn; *s; s++ )
            {
                /* building menu */

                char *path = strdup ( *s );

                unescape_url ( path );

                if( path == NULL)
                    continue;

                char *ns;
                asprintf ( &ns, "%s%s", a_peer_prefix, path );

                peer_menu->add ( ns, 0, NULL, const_cast<char*> ( *s ), 0 );

                free ( path );
                //            free(*s);
            }

            free ( conn );
        }
    }
}

/** build the context menu for this control */
Fl_Menu_Button &
Controller_Module::menu( void )
{
    static Fl_Menu_Button m ( 0, 0, 0, 0, "Controller" );

    m.clear ( );

    if ( mode ( ) == GUI )
    {
        add_osc_peers_to_menu ( &m, "Connect To" );
        add_osc_connections_to_menu ( &m, "Disconnect From" );
    }

    m.add ( "Mode/GUI + OSC", 0, 0, 0, FL_MENU_RADIO | ( mode ( ) == GUI ? FL_MENU_VALUE : 0 ) );
    m.add ( "Mode/Control Voltage (JACK)", 0, 0, 0, FL_MENU_RADIO | ( mode ( ) == CV ? FL_MENU_VALUE : 0 ) );
    m.add ( "Remove", 0, 0, 0, is_default ( ) ? FL_MENU_INACTIVE : 0 );

    //    menu_set_callback( m.items(), &Controller_Module::menu_cb, (void*)this );
    m.callback ( &Controller_Module::menu_cb, static_cast<void*> ( this ) );
    //   m.copy( items, (void*)this );

    return m;
}

void
Controller_Module::draw( void )
{
    Fl_Group::draw ( );
    draw_box ( x ( ), y ( ), w ( ), h ( ) );

    if ( learn_mode ( ) )
    {
#ifdef FLTK_SUPPORT
        // Since we don't have alpha transparency for FLTK, just draw 3 pixel outline,
        // each rectangle is one pixel
        fl_rect ( x ( ), y ( ), w ( ), h ( ),
            this == _learning_control
            ? FL_RED
            : FL_GREEN
        );

        fl_rect ( x ( ) + 1, y ( ) + 1, w ( ) - 2, h ( ) - 2,
            this == _learning_control
            ? FL_RED
            : FL_GREEN
        );

        fl_rect ( x ( ) + 2, y ( ) + 2, w ( ) - 4, h ( ) - 4,
            this == _learning_control
            ? FL_RED
            : FL_GREEN
        );
#else
        fl_rectf ( x ( ), y ( ), w ( ), h ( ),
            fl_color_add_alpha (
            this == _learning_control
            ? FL_RED
            : FL_GREEN,
            60 ) );
#endif
    }
}

void
Controller_Module::learning_callback( void *userdata )
{
    ( (Controller_Module*) userdata )->learning_callback ( );
}

void
Controller_Module::learning_callback( void )
{
    _learning_control = NULL;
    this->redraw ( );
}

int
Controller_Module::handle( int m )
{

    switch ( m )
    {
        case FL_PUSH:
        {
            if ( learn_mode ( ) )
            {
                tooltip ( "Now learning control. Move the desired control on your controller" );

                _learning_control = this;

                this->redraw ( );

                //connect_to( &module->control_input[port] );
                Port *p = control_output[0].connected_port ( );

                if ( p )
                {
                    const char * path = learn_by_number ? p->osc_number_path ( ) : p->osc_path ( );

                    DMESSAGE ( "Will learn %s", path );

                    mixer->osc_endpoint->learn ( path, Controller_Module::learning_callback, this );
                    mixer->redraw ( );
                }

                return 1;
            }

            if ( Fl::event_button3 ( ) )
            {
                /* context menu */
                /* if ( type() != SPATIALIZATION ) */
                menu_popup ( &menu ( ) );

                return 1;
            }
            else
                return Fl_Group::handle ( m );
        }
    }

    return Fl_Group::handle ( m );
}

void
Controller_Module::handle_control_changed( Port *p )
{
    /* ignore changes initiated while mouse is over widget */

    if ( type ( ) == SPATIALIZATION )
    {
        if ( Mixer::spatialization_console )
            Mixer::spatialization_console->handle_control_changed ( this );
    }

    if ( contains ( Fl::pushed ( ) ) )
        return;

    if ( p )
        control_value = p->control_value ( );

    if ( type ( ) == CHOICE || type ( ) == TOGGLE )
    {
        // We have to check these always since the control value may not be the same as the widget value
    }
    else if ( ( (Fl_Valuator * ) control )->value ( ) == control_value )
        return;

    /* if ( control->value() != control_value ) */
    /* { */
    /*     redraw(); */
    /* } */

    if ( type ( ) == SPATIALIZATION )
    {
        Panner *pan = static_cast<Panner*> ( control );

        pan->point ( 0 )->azimuth ( control_output[0].control_value ( ) );
        pan->point ( 0 )->elevation ( control_output[1].control_value ( ) );

        if ( control_output[2].connected ( ) )
        {
            //            Port *pp = control_output[2].connected_port();
            float v = control_output[2].control_value ( );
            //            float s = pp->hints.maximum - pp->hints.minimum;

            pan->point ( 0 )->radius ( v );
        }
        if ( visible_r ( ) )
            pan->redraw ( );
    }
    else
    {
        if ( type ( ) == TOGGLE )
        {
            ( (Fl_Button*) control )->value ( control_value );
        }
#ifdef LV2_SUPPORT
        else if ( type ( ) == CHOICE )
        {
            // DMESSAGE("control_value = %f: size = %d", control_value, p->hints.ScalePoints.size());
            /* We set the Fl_Choice menu according to the position in the ScalePoints vector */
            int menu_location = 0;

            for ( unsigned i = 0; i < p->hints.ScalePoints.size ( ); ++i )
            {
                if ( (int) p->hints.ScalePoints[i].Value == (int) ( control_value + .5 ) ) // .5 for float rounding
                {
                    menu_location = i;
                    break;
                }
            }

            ( (Fl_Choice*) control )->value ( menu_location );
        }
#endif  // LV2_SUPPORT
        else
            ((Fl_Valuator*) control )->value ( control_value );
    }
}

void
Controller_Module::command_remove( void )
{
    if ( is_default ( ) )
        fl_alert ( "Default modules may not be deleted." );
    else
    {
        chain ( )->remove ( this );
        Fl::delete_widget ( this );
    }
}

/**********/
/* Client */

/**********/

void
Controller_Module::process( nframes_t nframes )
{
    THREAD_ASSERT ( RT );

    if ( type ( ) == SPATIALIZATION )
    {
        return;
    }

    if ( control_output[0].connected ( ) )
    {
        float f = control_value;

        if ( mode ( ) == CV )
        {
            f = *( static_cast<float*> ( aux_audio_input[0].jack_port ( )->buffer ( nframes ) ) );

            const Port *p = control_output[0].connected_port ( );

            if ( p->hints.ranged )
            {
                // scale value to range.
                // we assume that CV values are between 0 and 1

                float scale = p->hints.maximum - p->hints.minimum;
                float offset = p->hints.minimum;

                f = ( f * scale ) + offset;
            }
        }
        //        else
        //            f =  *((float*)control_output[0].buffer());

        *( static_cast<float*> ( control_output[0].buffer ( ) ) ) = f;

        control_value = f;
    }
}
