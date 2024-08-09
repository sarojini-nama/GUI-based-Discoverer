#include <string.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gio/gio.h> // For GFile

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

GtkTextBuffer *text_buffer;

typedef struct _CustomData 
{
	GstDiscoverer *discoverer;
}CustomData;

//to print on text view
void printing_on_text_view(gchar *text_print_data)
{
	GtkTextIter end_iter;
	gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
	gtk_text_buffer_insert(text_buffer, &end_iter, text_print_data, -1);
}

//clears the log
static void on_button_log_clear_clicked()
{
	gtk_text_buffer_set_text(text_buffer, "", -1);
}

//storing the log details in a file
static void on_button_store_log_file_clicked(GtkButton *button, gpointer user_data)
{
	GtkEntry *entry = GTK_ENTRY(user_data);
	GtkEntryBuffer *buffer = gtk_entry_get_buffer(entry);
	const gchar *filename = gtk_entry_buffer_get_text(buffer);
	GtkTextIter start, end;
	gchar *text;
	FILE *file;
	const gchar *filepath;
	const char *username;
	
	username = getenv("USER");
	if(username == NULL)
	{
		username = getenv("LOGNAME");
	}
	
	filepath = g_strdup_printf("/home/%s/Downloads/%s", username, filename);
	
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	
	text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
	
	file = fopen(filepath, "w");
	if (file == NULL) 
	{
		return;
	}
	fprintf(file, "%s", text);
	
	fclose(file);
	
	g_free(text);
}

/* Print a tag in a human-readable format (name: value) */
static void print_tag_foreach (const GstTagList *tags, const gchar *tag, gpointer user_data) 
{
	GValue val = { 0, };
	gchar *str;
	gchar *text_print_data;
	gint depth = GPOINTER_TO_INT (user_data);

	gst_tag_list_copy_value (&val, tags, tag);

	if (G_VALUE_HOLDS_STRING (&val))
		str = g_value_dup_string (&val);
	else
		str = gst_value_serialize (&val);

	text_print_data = g_strdup_printf("%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick (tag), str);
	printing_on_text_view(text_print_data);
	
	g_free(text_print_data);
	g_free (str);

	g_value_unset (&val);
}

/* Print information regarding a stream */
static void print_stream_info (GstDiscovererStreamInfo *info, gint depth) 
{
	gchar *desc = NULL;
	GstCaps *caps;
	const GstTagList *tags;
	
	gchar *text_print_data;

	caps = gst_discoverer_stream_info_get_caps (info);

	if (caps) {
		if (gst_caps_is_fixed (caps))
			desc = gst_pb_utils_get_codec_description (caps);
		else
			desc = gst_caps_to_string (caps);
		gst_caps_unref (caps);
	}

	text_print_data = g_strdup_printf("%*s%s: %s\n", 2 * depth, " ", gst_discoverer_stream_info_get_stream_type_nick (info), (desc ? desc : ""));
	printing_on_text_view(text_print_data);

	if (desc) {
		g_free (desc);
		desc = NULL;
	}

	tags = gst_discoverer_stream_info_get_tags (info);
	if (tags) {
		text_print_data = g_strdup_printf("%*sTags:\n", 2 * (depth + 1), " ");
		printing_on_text_view(text_print_data);
		gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (depth + 2));
	}
	g_free(text_print_data);
}

/* Print information regarding a stream and its substreams, if any */
static void print_topology (GstDiscovererStreamInfo *info, gint depth) 
{
	GstDiscovererStreamInfo *next;

	if (!info)
		return;

	print_stream_info (info, depth);

	next = gst_discoverer_stream_info_get_next (info);
	if (next) {
		print_topology (next, depth + 1);
		gst_discoverer_stream_info_unref (next);
	} else if (GST_IS_DISCOVERER_CONTAINER_INFO (info)) {
		GList *tmp, *streams;

		streams = gst_discoverer_container_info_get_streams (GST_DISCOVERER_CONTAINER_INFO (info));
		for (tmp = streams; tmp; tmp = tmp->next) {
			GstDiscovererStreamInfo *tmpinf = (GstDiscovererStreamInfo *) tmp->data;
			print_topology (tmpinf, depth + 1);
		}
		gst_discoverer_stream_info_list_free (streams);
	}
}

/* This function is called every time the discoverer has information regarding
 * one of the URIs we provided.*/
static void on_discovered_cb (GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, CustomData *data) 
{
	GstDiscovererResult result;
	const gchar *uri;
	const GstTagList *tags;
	GstDiscovererStreamInfo *sinfo;
	
	gchar *text_print_data;

	uri = gst_discoverer_info_get_uri (info);
	result = gst_discoverer_info_get_result (info);
	
	switch (result) {
		case GST_DISCOVERER_URI_INVALID:
			text_print_data = g_strdup_printf("Invalid URI '%s'\n", uri);
			printing_on_text_view(text_print_data);
			break;
		case GST_DISCOVERER_ERROR:
			text_print_data = g_strdup_printf("Discoverer error: %s\n", err->message);
			printing_on_text_view(text_print_data);
			break;
		case GST_DISCOVERER_TIMEOUT:
			text_print_data = g_strdup_printf("Timeout\n");
			printing_on_text_view(text_print_data);
			break;
		case GST_DISCOVERER_BUSY:
			text_print_data = g_strdup_printf("Busy\n");
			printing_on_text_view(text_print_data);
			break;
		case GST_DISCOVERER_MISSING_PLUGINS:{
							    const GstStructure *s;
							    gchar *str;

							    s = gst_discoverer_info_get_misc (info);
							    str = gst_structure_to_string (s);

							    text_print_data = g_strdup_printf("Missing plugins: %s\n", str);
							    printing_on_text_view(text_print_data);
							    g_free (str);
							    break;
						    }
		case GST_DISCOVERER_OK:
						    text_print_data = g_strdup_printf("Discovered '%s'\n", uri);
						    printing_on_text_view(text_print_data);
						    break;
	}

	if (result != GST_DISCOVERER_OK) {
		text_print_data = g_strdup_printf("This URI cannot be played\n");
		printing_on_text_view(text_print_data);
		return;
	}

	/* If we got no error, show the retrieved information */
	text_print_data = g_strdup_printf("\nDuration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (gst_discoverer_info_get_duration (info)));
	printing_on_text_view(text_print_data);

	tags = gst_discoverer_info_get_tags (info);
	if (tags) {
		text_print_data = g_strdup_printf("Tags:\n");
		printing_on_text_view(text_print_data);
		gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (1));
	}
	text_print_data = g_strdup_printf("Seekable: %s\n", (gst_discoverer_info_get_seekable (info) ? "yes" : "no"));
	printing_on_text_view(text_print_data);

	text_print_data = g_strdup_printf("\n");
	printing_on_text_view(text_print_data);

	sinfo = gst_discoverer_info_get_stream_info (info);
	if (!sinfo)
		return;

	text_print_data = g_strdup_printf("Stream information:\n");
	printing_on_text_view(text_print_data);
	print_topology(sinfo, 1);

	gst_discoverer_stream_info_unref (sinfo);

	text_print_data = g_strdup_printf("\n");
	printing_on_text_view(text_print_data);
	
	g_free(text_print_data);
}

/* This function is called when the discoverer has finished examining
 * all the URIs we provided.*/
static void on_finished_cb (GstDiscoverer *discoverer, CustomData *data) 
{
	gchar *text_print_data = g_strdup_printf("Finished discovering\n***\n\n");
	printing_on_text_view(text_print_data);
	
	g_free(text_print_data);
}

//performing the discoverer operation
static int discovering(const gchar *uri, GError *err)
{
	CustomData data;

	gchar *text_print_data;
	
	text_print_data = g_strdup_printf("Discovering '%s'\n\n", uri);
	printing_on_text_view(text_print_data);
	
		/* Instantiate the Discoverer */
		data.discoverer = gst_discoverer_new (5 * GST_SECOND, &err);
		if (!data.discoverer) {
			text_print_data = g_strdup_printf("Error creating discoverer instance: %s\n", err->message);
			printing_on_text_view(text_print_data);
			g_clear_error (&err);
			return -1;
		}

		/* Connect to the interesting signals */
		g_signal_connect (data.discoverer, "discovered", G_CALLBACK (on_discovered_cb), &data);
		g_signal_connect (data.discoverer, "finished", G_CALLBACK (on_finished_cb), &data);

		/* Start the discoverer process (nothing to do yet) */
		gst_discoverer_start (data.discoverer);

		/* Add a request to process asynchronously the URI passed through the command line */
		if (!gst_discoverer_discover_uri_async (data.discoverer, uri)) {
			text_print_data = g_strdup_printf("Failed to start discovering URI '%s'\n", uri);
			printing_on_text_view(text_print_data);
			g_object_unref (data.discoverer);
			return -1;
		}
	g_free(text_print_data);
}

//for the files in the system
static void on_file_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) 
{
	gchar *text_print_data;

	if (response_id == GTK_RESPONSE_ACCEPT) 
	{

		CustomData data;
		const gchar *uri;
		GError *err = NULL;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		GFile *file = gtk_file_chooser_get_file(chooser); // Get GFile
		gchar *filename = g_file_get_path(file); // Get the absolute path
		
		text_print_data = g_strdup_printf("Selected file: %s\n", filename);
		printing_on_text_view(text_print_data);
		uri = gst_filename_to_uri(filename, NULL);
		
		discovering(uri, err);

		g_free(filename);
		g_object_unref(file);
	} 
	else 
	{
		text_print_data = g_strdup_printf("\n***\nYou pressed cancel\n***\n");
		printing_on_text_view(text_print_data);
	}
	g_free(text_print_data);
	gtk_window_destroy(GTK_WINDOW(dialog)); // Close the dialog
}

//for browser search
static void on_button_browser_clicked(GtkButton *button, gpointer user_data)
{
	CustomData data;
	GError *err = NULL;
	GtkEntry *entry = GTK_ENTRY(user_data);
	GtkEntryBuffer *buffer = gtk_entry_get_buffer(entry);
	const gchar *uri = gtk_entry_buffer_get_text(buffer);
	
	discovering(uri, err);
}

//opening file chooser
static void open_dialog(GtkWidget *button, gpointer window) 
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Choose a file",
			GTK_WINDOW(window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"_Cancel", GTK_RESPONSE_CANCEL,
			"_Open", GTK_RESPONSE_ACCEPT,
			NULL);
	g_signal_connect(dialog, "response", G_CALLBACK(on_file_dialog_response), NULL);
	gtk_widget_show(dialog);
}

//UI
static void create_ui(GtkApplication *app, gpointer user_data) 
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *box_title, *box_file, *box_browser, *box_output, *box_log_clear, *box_store_log_file;
	GtkWidget *entry_browser, *entry_store_log_file;
	GtkWidget *button_file, *button_browser, *button_log_clear, *button_store_log_file;
	GtkWidget *label_in_box_file, *label_in_box_browser, *label_in_box_store_log_file;
	GtkWidget *title_label_in_box_title, *title_label_in_box_file, *title_label_in_box_browser, *title_label_in_box_output;
	GtkWidget *text_view;
	GtkWidget *scrolled_window;

	//window
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Basic Discoverer");
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);

	//grid
	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 30);

	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);

	//box
	box_title = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	box_file = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	box_browser = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	box_output = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	box_log_clear = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	box_store_log_file = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	gtk_widget_set_hexpand(box_title, TRUE);
	gtk_widget_set_vexpand(box_title, TRUE);
	gtk_widget_set_hexpand(box_file, TRUE);
	gtk_widget_set_vexpand(box_file, TRUE);
	gtk_widget_set_hexpand(box_browser, TRUE);
	gtk_widget_set_vexpand(box_browser, TRUE);
	gtk_widget_set_hexpand(box_output, TRUE);
	gtk_widget_set_vexpand(box_output, TRUE);
	gtk_widget_set_hexpand(box_log_clear, TRUE);
	gtk_widget_set_vexpand(box_log_clear, TRUE);
	gtk_widget_set_hexpand(box_store_log_file, TRUE);
	gtk_widget_set_vexpand(box_store_log_file, TRUE);

	//box_title
	gtk_widget_set_margin_start(box_title, 10);
	gtk_widget_set_margin_end(box_title, 10);
	gtk_widget_set_margin_top(box_title, 10);
	gtk_widget_set_margin_bottom(box_title, 10);
	
	//box_file
	gtk_widget_set_margin_start(box_file, 10);
	gtk_widget_set_margin_end(box_file, 10);
	gtk_widget_set_margin_top(box_file, 10);
	gtk_widget_set_margin_bottom(box_file, 10);

	//box_browser
	gtk_widget_set_margin_start(box_browser, 10);
	gtk_widget_set_margin_end(box_browser, 10);
	gtk_widget_set_margin_top(box_browser, 10);
	gtk_widget_set_margin_bottom(box_browser, 10);
	
	//box_output
	gtk_widget_set_margin_start(box_output, 10);
	gtk_widget_set_margin_end(box_output, 10);
	gtk_widget_set_margin_top(box_output, 10);
	gtk_widget_set_margin_bottom(box_output, 10);
	
	//box_log_clear
	gtk_widget_set_margin_start(box_log_clear, 10);
	gtk_widget_set_margin_end(box_log_clear, 10);
	gtk_widget_set_margin_top(box_log_clear, 10);
	gtk_widget_set_margin_bottom(box_log_clear, 10);
	
	//box_store_log_file
	gtk_widget_set_margin_start(box_store_log_file, 10);
	gtk_widget_set_margin_end(box_store_log_file, 10);
	gtk_widget_set_margin_top(box_store_log_file, 10);
	gtk_widget_set_margin_bottom(box_store_log_file, 10);
	
	//box_title
	title_label_in_box_title = gtk_label_new("BASIC DISCOVERER");
	
	gtk_widget_set_halign(title_label_in_box_title, GTK_ALIGN_CENTER);
	
	gtk_box_append(GTK_BOX(box_title), title_label_in_box_title);

	//box_file
	title_label_in_box_file = gtk_label_new("TO DISCOVERER THE FILES IN SYSTEM");
	label_in_box_file = gtk_label_new("Choose the file to be Discovered");
	button_file = gtk_button_new_with_label("Select File");
	g_signal_connect(button_file, "clicked", G_CALLBACK(open_dialog), window);

	gtk_widget_set_halign(title_label_in_box_file, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(label_in_box_file, GTK_ALIGN_START);

	gtk_box_append(GTK_BOX(box_file), title_label_in_box_file);
	gtk_box_append(GTK_BOX(box_file), label_in_box_file);
	gtk_box_append(GTK_BOX(box_file), button_file);

	//box_browser
	title_label_in_box_browser = gtk_label_new("TO DISCOVERER THE URIS PROVIDED");
	label_in_box_browser = gtk_label_new("Discoverer for the uri");

	entry_browser = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry_browser), "Enter the URI here...");

	button_browser = gtk_button_new_with_label("Submit to Discoverer");
	g_signal_connect(button_browser, "clicked", G_CALLBACK(on_button_browser_clicked), entry_browser);

	gtk_widget_set_halign(title_label_in_box_browser, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(label_in_box_browser, GTK_ALIGN_START);

	gtk_box_append(GTK_BOX(box_browser), title_label_in_box_browser);
	gtk_box_append(GTK_BOX(box_browser), label_in_box_browser);
	gtk_box_append(GTK_BOX(box_browser), entry_browser);
	gtk_box_append(GTK_BOX(box_browser), button_browser);
	
	//box_output
	title_label_in_box_output = gtk_label_new("METADATA OF THE SEARCHED FILES");
	text_view = gtk_text_view_new();
	text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	
	scrolled_window = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolled_window, 400, 300);
	
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), text_view);
	
	gtk_widget_set_halign(title_label_in_box_output, GTK_ALIGN_CENTER);
	
	gtk_box_append(GTK_BOX(box_output), title_label_in_box_output);
	gtk_box_append(GTK_BOX(box_output), scrolled_window);
	
	//box_log_clear
	button_log_clear = gtk_button_new_with_label("Clear the discoverered log");
	g_signal_connect(button_log_clear, "clicked", G_CALLBACK(on_button_log_clear_clicked), NULL);
	
	gtk_box_append(GTK_BOX(box_log_clear), button_log_clear);
	
	label_in_box_store_log_file = gtk_label_new("To save the discoverered logs currently present in text view\nEnter the filename with your preffered name... It will be stored in /Downloads");
	gtk_widget_set_halign(label_in_box_store_log_file, GTK_ALIGN_START);
	
	//box_store_log_file
	entry_store_log_file = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry_store_log_file), "Enter the filename and add .txt at the end...");
	
	button_store_log_file = gtk_button_new_with_label("Download the file");
	g_signal_connect(button_store_log_file, "clicked", G_CALLBACK(on_button_store_log_file_clicked), entry_store_log_file);
	
	gtk_box_append(GTK_BOX(box_store_log_file), label_in_box_store_log_file);
	gtk_box_append(GTK_BOX(box_store_log_file), entry_store_log_file);
	gtk_box_append(GTK_BOX(box_store_log_file), button_store_log_file);
	
	//grid_position_details
	gtk_grid_attach(GTK_GRID(grid), box_title, 0, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), box_file, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), box_browser, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), box_output, 0, 2, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), box_log_clear, 0, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), box_store_log_file, 1, 3, 1, 1);

	gtk_window_set_child(GTK_WINDOW(window), grid);

	gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) 
{
	GtkApplication *app;
	int status;

	CustomData data;

	memset (&data, 0, sizeof (data));

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(create_ui), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}    
