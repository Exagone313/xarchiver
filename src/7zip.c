/*
 *  Xarchiver
 *
 *  Copyright (C) 2005 Giuseppe Torelli - Colossus
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */
 
#include "7zip.h"
 
extern int output_fd,error_fd,child_pid,child_status;

void Open7Zip ( gboolean mode , gchar *path)
{
    jump_header = FALSE;
	gchar *command = g_strconcat ( "7za l " , path, NULL );
	compressor_pid = SpawnAsyncProcess ( command , 1 , 0);
	g_free ( command );
	if ( compressor_pid == 0 ) return;
	char *names[]= {(_("Filename")),(_("Compressed")),(_("Original")),(_("Attr")),(_("Time")),(_("Date"))};
	GType types[]= {G_TYPE_STRING,G_TYPE_UINT,G_TYPE_UINT,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING};
	CreateListStore ( 6, names , (GType *)types );
	SetIOChannel (output_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,SevenZipOpen, (gpointer) mode );
	SetIOChannel (error_fd, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,GenError, NULL );
    WaitExitStatus ( child_pid , NULL );
}

static gboolean SevenZipOpen (GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	gchar **fields = NULL;
	gchar *filename = NULL;
	gchar *line;
	if (cond & (G_IO_IN | G_IO_PRI) )
	{
        //This to avoid inserting in the liststore 7zip's message
        if (jump_header == FALSE )
        {
			for ( x = 0; x <= 7; x++)
			{
				g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
                if (line != NULL && data ) gtk_text_buffer_insert(textbuf, &enditer, line, strlen ( line ) );
                g_free (line);
			}
			jump_header = TRUE;
        }
		g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
        //This to avoid inserting the last line of output
        if (strncmp (line, "-------------------", 19) == 0 || strncmp (line, "\x0a",1) == 0)
		{
            g_free (line);
			g_io_channel_read_line ( ioc, &line, NULL, NULL, NULL );
            if ( line != NULL && data )
            {
                gtk_text_buffer_insert (textbuf, &enditer, line, strlen( line ) );
                g_free (line);
            }
            return TRUE;
		}
		if (line != NULL && data ) gtk_text_buffer_insert (textbuf, &enditer, line, strlen ( line ) );
		fields = split_line ( line , 5 );
        filename = get_last_field ( line , 6);
		gtk_list_store_append (liststore, &iter);
        for ( x = 0; x < 5; x++)
		{
            if ( x == 3 || x == 4) gtk_list_store_set (liststore, &iter,(5-x),atoi(fields[x]),-1);
                else gtk_list_store_set (liststore, &iter,(5-x),fields[x],-1);
		}
        if ( filename == NULL ) gtk_list_store_set (liststore, &iter,0,fields[4],-1);
        else gtk_list_store_set (liststore, &iter,0,filename,-1);
        gtk_progress_bar_pulse ( GTK_PROGRESS_BAR (progressbar) );
        while (gtk_events_pending() )
	        gtk_main_iteration();
		g_strfreev ( fields );
		g_free (line);
		return TRUE;
	}
	else if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
}


