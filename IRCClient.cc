
#include <stdio.h>
#include <gtk/gtk.h>
#include "IRCNetworking.h"

using namespace std;

GtkWidget *myMessage;
GtkListStore * list_rooms;
GtkListStore * list_users;
GtkWidget *window;
GtkTextBuffer *buffer;
GtkWidget *myMessageBox;


IRCNetworking* net = NULL;

string currentRoom = "";

vector<string>* listOfRooms = NULL;
vector<string>* listOfUsers = NULL;
vector<IRCMessage>* listOfMessages = NULL;

void update_list_rooms() {
	GtkTreeIter iter;
	gtk_list_store_clear(GTK_LIST_STORE (list_rooms));

	if(listOfRooms == NULL) {
		// no rooms

		return;
	}

	int k = listOfRooms->size();
	int i;
	/* Add some messages to the window */
	for (i = 0; i < k; i++) {
		gchar *msg = g_strdup_printf ("%s", (*listOfRooms)[i].c_str());
		printf("room: %s\n", msg);
		gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
		gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter,0, msg,-1);
		g_free (msg);
	}
}

void update_list_users() {
	GtkTreeIter iter;
	gtk_list_store_clear(GTK_LIST_STORE (list_users));

	if(listOfUsers == NULL) {
		// no rooms

		return;
	}

	int k = listOfUsers->size();

	int i;
	/* Add some messages to the window */
	for (i = 0; i < k; i++) {
	 	gchar *msg = g_strdup_printf ("%s", (*listOfUsers)[i].c_str());
		printf("user in room: %s\n", msg);
	    gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
	    gtk_list_store_set (GTK_LIST_STORE (list_users), &iter,0, msg,-1);
		g_free (msg);
	}

}

void update_message() {
	GtkTextIter ei;


	printf("buffer: %p listOfMessages %p\n", buffer, listOfMessages);
	gtk_text_buffer_set_text(buffer, "", -1);

	if(listOfMessages == NULL) {
		return;
	}

	int length = listOfMessages->size();

	for(int i = 0 ; i < length; i++) {
		gtk_text_buffer_get_end_iter(buffer, &ei);

		string text = (*listOfMessages)[i].user + ": " + (*listOfMessages)[i].message + "\n";

		gtk_text_buffer_insert(buffer, &ei, text.c_str(), -1);
	}

	
}

void refresh() {
	// update

	// update list of rooms


	IRCResponse* info = net->listRooms();
	listOfRooms = info->data;

	if(currentRoom.length() == 0) {
		update_list_rooms();
		return;
	}

	// update list of users in current room

	IRCResponse* usersIn = net->getUsersInRoom(currentRoom);
	listOfUsers = usersIn->data;

	// update messages
	int lastNum = 0;
	if(listOfMessages != NULL && listOfMessages->size() > 0) {
		lastNum = listOfMessages->back().messageNum + 1;
	}

	IRCMessageResponse* msgs = net->getMessages(currentRoom, lastNum);


	listOfMessages->insert(listOfMessages->end(), msgs->messages->begin(), msgs->messages->end());

	update_message();
	update_list_users();
	update_list_rooms();
}

static gboolean time_handler(GtkWidget *widget) {
  if (widget->window == NULL) return FALSE;

  refresh();

  gtk_widget_queue_draw(widget);
  return TRUE;
}

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model, GtkWidget **extr_tree_view )
{
	GtkWidget *scrolled_window;
	GtkWidget *tree_view;
	//GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;

	int i;
   
	/* Create a new scrolled window, with scrollbars only if needed */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   
	//model = gtk_list_store_new (1, G_TYPE_STRING);
	tree_view = gtk_tree_view_new ();
	*extr_tree_view = tree_view;
	gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
	gtk_widget_show (tree_view);
   
	cell = gtk_cell_renderer_text_new ();

	column = gtk_tree_view_column_new_with_attributes (titleColumn,cell,"text", 0,NULL);
  
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),GTK_TREE_VIEW_COLUMN (column));

	return scrolled_window;
}
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static void insert_text( GtkTextBuffer *buffer, const char * initialText )
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText, int editable, GtkTextBuffer **buffr, GtkWidget **wid )
{
	GtkWidget *scrolled_window;
	GtkWidget *view;

	view = gtk_text_view_new ();

	if(wid != NULL) {
		*wid = view;
	}
	
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), editable);

	*buffr = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scrolled_window), view);
	insert_text (*buffr, initialText);

	gtk_widget_show_all (scrolled_window);

	return scrolled_window;
}

void createRoomClicked (GtkButton *button, gpointer user_data) {
	printf("clicked roombutton\n");

	GtkWidget *signOn;
	GtkWidget *userNameEntry;
	GtkEntryBuffer *userBuffer;
	GtkEntryBuffer *passwordBuffer;
	GtkWidget *box;
	GtkWidget *usernameLabel;
	GtkWidget *passwordLabel;

	signOn = gtk_message_dialog_new (GTK_WINDOW (window), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (signOn), "Create Room", 1, NULL);
	userBuffer = gtk_entry_buffer_new (NULL, -1);

	usernameLabel = gtk_label_new ("Enter Room Name");


	userNameEntry = gtk_entry_new_with_buffer (userBuffer);
	box = gtk_dialog_get_content_area (GTK_DIALOG (signOn));
	gtk_box_pack_end(GTK_BOX (box), userNameEntry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), usernameLabel, FALSE, FALSE, 0);

	gtk_widget_show_all(signOn);
a_new_two:
	int result = gtk_dialog_run(GTK_DIALOG (signOn));
	gtk_widget_destroy (signOn);

	const char * roomName = gtk_entry_buffer_get_text (userBuffer);


	if(result == 1) { // create room clicked 
		IRCResponse* tester = net->createRoom(roomName);

		if(tester->okay == FALSE) {
			// didn't work
			goto a_new_two;

		}
	}
}

void roomListSelection(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model;
    GtkTreeIter   iter;
 
    model = gtk_tree_view_get_model(tree_view);
 
    if (gtk_tree_model_get_iter(model, &iter, path)) {
    	gchar *name;

		gtk_tree_model_get(model, &iter, 0, &name, -1);

		if(currentRoom.compare("") != 0) {
			net->sendMessage(currentRoom, "has left the room");

			net->leaveRoom(currentRoom);
		}

		currentRoom = string (name);

		listOfMessages = new vector<IRCMessage>();

		net->enterRoom(currentRoom);

		net->sendMessage(currentRoom, "has entered the room");

		g_free(name);

		refresh();
	}
}

void createAccountClicked (GtkButton *button, gpointer user_data) {
	printf("clicked account button\n");
	
	GtkWidget *signOn;
	GtkWidget *userNameEntry;
	GtkWidget *passwordEntry;
	GtkEntryBuffer *userBuffer;
	GtkEntryBuffer *passwordBuffer;
	GtkWidget *box;
	GtkWidget *usernameLabel;
	GtkWidget *passwordLabel;

	signOn = gtk_message_dialog_new (GTK_WINDOW (window), (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (signOn), "Sign Up", 1, NULL);
	userBuffer = gtk_entry_buffer_new (NULL, -1);
	passwordBuffer = gtk_entry_buffer_new (NULL, -1);

	usernameLabel = gtk_label_new ("Enter Username");
	passwordLabel = gtk_label_new ("Enter Password");


	userNameEntry = gtk_entry_new_with_buffer (userBuffer);
	passwordEntry = gtk_entry_new_with_buffer (passwordBuffer);
	gtk_entry_set_visibility (GTK_ENTRY (passwordEntry), FALSE);
	box = gtk_dialog_get_content_area (GTK_DIALOG (signOn));
	gtk_box_pack_end(GTK_BOX (box), passwordEntry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), passwordLabel, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), userNameEntry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), usernameLabel, FALSE, FALSE, 0);

	gtk_widget_show_all(signOn);
a_new_one:
	int result = gtk_dialog_run(GTK_DIALOG (signOn));
	gtk_widget_destroy (signOn);

	const char * username = gtk_entry_buffer_get_text (userBuffer);
	const char * password = gtk_entry_buffer_get_text (passwordBuffer);



	if(result == 1) { // Signing up 
		IRCResponse* tester = net->addUser(username, password);

		if(tester->okay == FALSE) {
			// didn't work
			goto a_new_one;

		}
	}



}

void errorMessage (string error) {

	GtkWidget *signOn;


	signOn = gtk_message_dialog_new (GTK_WINDOW (window), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", error.c_str());

	int result = gtk_dialog_run(GTK_DIALOG (signOn));
	gtk_widget_destroy (signOn);

}

void sendMessageClicked (GtkButton *button, gpointer user_data) {

	if(currentRoom.length() > 0) {
		GtkTextIter start, end;

		GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(myMessageBox));
		gchar *text;
		gtk_text_buffer_get_bounds (buf, &start, &end);

		text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);


		IRCResponse* sender = net->sendMessage(currentRoom, text);

		gtk_text_buffer_set_text(buf, "*insert message here*", -1);

		if(sender->okay == FALSE) {
			// message not sent
		}

		refresh();

	}

	



}

int main( int   argc,
		  char *argv[] )
{
	GtkWidget *signOn;
	GtkWidget *userNameEntry;
	GtkWidget *passwordEntry;
	GtkEntryBuffer *userBuffer;
	GtkEntryBuffer *passwordBuffer;
	GtkWidget *list;
	GtkWidget *list2;
	GtkWidget *messages;
	GtkWidget *box;
	GtkWidget *usernameLabel;
	GtkWidget *passwordLabel;

	GtkWidget *createRoom;
	GtkWidget *roomEntry;

	gtk_init (&argc, &argv);

	if(argc < 2) {
		printf("Incorrect arguments.  Must enter port\n");
		return 1;
	}

	int portNum = atoi(argv[1]);

	net = new IRCNetworking(portNum);
   
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Paned Windows");
	g_signal_connect (window, "destroy",G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	gtk_widget_set_size_request (GTK_WIDGET (window), 450, 400);
	
	//Sign in or Sign Up Window

	signOn = gtk_message_dialog_new (GTK_WINDOW (window), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (signOn), "Sign In", 1, "Sign Up", 2, NULL);
	userBuffer = gtk_entry_buffer_new (NULL, -1);
	passwordBuffer = gtk_entry_buffer_new (NULL, -1);

	usernameLabel = gtk_label_new ("Enter Username");
	passwordLabel = gtk_label_new ("Enter Password");


	userNameEntry = gtk_entry_new_with_buffer (userBuffer);
	passwordEntry = gtk_entry_new_with_buffer (passwordBuffer);
	gtk_entry_set_visibility (GTK_ENTRY (passwordEntry), FALSE);
	box = gtk_dialog_get_content_area (GTK_DIALOG (signOn));
	gtk_box_pack_end(GTK_BOX (box), passwordEntry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), passwordLabel, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), userNameEntry, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (box), usernameLabel, FALSE, FALSE, 0);



	//Create Room Window

	//createRoom = gtk_message_dialog_new (GTK_WINDOW (window), )


	// Create a table to place the widgets. Use a 7x4 Grid (7 rows x 4 columns)
	GtkWidget *table = gtk_table_new (7, 4, TRUE);
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_table_set_row_spacings(GTK_TABLE (table), 5);
	gtk_table_set_col_spacings(GTK_TABLE (table), 5);
	gtk_widget_show (table);

	// Add list of rooms. Use columns 0 to 4 (exclusive) and rows 0 to 4 (exclusive)
	list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
	update_list_rooms();

	GtkWidget *roomsTreeView = NULL;
	list = create_list ("Rooms", list_rooms, &roomsTreeView);
	gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 2, 0, 2);
	gtk_widget_show (list);
	g_signal_connect(GTK_OBJECT(roomsTreeView), "row-activated", G_CALLBACK (&roomListSelection), NULL);

	GtkWidget *stuff;
   
	// Add users list for the room
	list_users = gtk_list_store_new (1, G_TYPE_STRING);
	update_list_users();
	list2 = create_list ("Users", list_users, &stuff);
	gtk_table_attach_defaults (GTK_TABLE (table), list2, 2, 4, 0, 2);
	gtk_widget_show (list2);

	// Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
	messages = create_text ("", 0, &buffer, NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), messages, 0, 4, 2, 5);
	gtk_widget_show (messages);
	// Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 

	GtkTextBuffer *bb;
	myMessage = create_text ("*insert message here*", 1, &bb, &myMessageBox);
	gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 0, 4, 5, 7);
	gtk_widget_show (myMessage);

	// Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
	GtkWidget *send_button = gtk_button_new_with_label ("Send");
	g_signal_connect(GTK_OBJECT(send_button), "clicked", G_CALLBACK (&sendMessageClicked), NULL);
	gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 2, 7, 8); 
	gtk_widget_show (send_button);
	

	// Add a Create Room button
	
	GtkWidget *room_button = gtk_button_new_with_label ("Create Room");
	g_signal_connect(GTK_OBJECT(room_button), "clicked", G_CALLBACK (&createRoomClicked), NULL);
	gtk_table_attach_defaults(GTK_TABLE (table), room_button, 2, 3, 7, 8);
	gtk_widget_show (room_button);

	// Create an Account button
	GtkWidget *account_button = gtk_button_new_with_label ("Create Account");
	g_signal_connect(GTK_OBJECT(account_button), "clicked", G_CALLBACK (&createAccountClicked), NULL);
	gtk_table_attach_defaults(GTK_TABLE (table), account_button, 3, 4, 7, 8);
	gtk_widget_show (account_button);


	gtk_widget_show (table);
	gtk_widget_show (window);
	gtk_widget_show_all(signOn);

show_dialog:
	int result = gtk_dialog_run(GTK_DIALOG (signOn));
	gtk_widget_hide (signOn);

	const char * username1 = gtk_entry_buffer_get_text (userBuffer);
	const char * password1 = gtk_entry_buffer_get_text (passwordBuffer);



	if(result == 1) { // Signing In 
		net->username = username1;
		net->password = password1;

		IRCResponse* test = net->listRooms();

		if(test->okay == FALSE) {
			// command didn't work
			// Wrong user name or password

			string error = "Incorrect Username or Password";
			errorMessage(error);
			goto show_dialog;
		}

		 g_timeout_add(5000, (GSourceFunc) time_handler, (gpointer) window);
		 refresh();
	}

	else if (result == 2) { // Signing Up
		net->username = username1;
		net->password = password1;

		IRCResponse* test = net->addUser(net->username, net->password);

		if(test->okay == FALSE) {
			// command didn't work

			string error = "Incorrect Username or Password";
			errorMessage(error);
			goto show_dialog;
		}

		g_timeout_add(5000, (GSourceFunc) time_handler, (gpointer) window);
		refresh();
	}

	else { // Didn't sign in - close out of windows
		gtk_widget_destroy(window);
		return 1;
	}


	gtk_main ();

	return 0;
}
