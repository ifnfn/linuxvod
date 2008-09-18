#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <time.h>

#include <glib.h>
#include <loudmouth/loudmouth.h>

#include <sqlite.h>

#include "crypt/hd.h"
#include "crypt/des.h"

static GMainLoop *main_loop = NULL;
static gboolean   test_success = FALSE;

static gchar      expected_fingerprint[20];

static gchar *server      = "talk.google.com";
static gint   port        = 5222;
static gchar *username    = "zzgmtv@gmail.com";
static gchar *password    = "cnsczd";
static gchar *resource    = "google talk";
static gchar *fingerprint = "f";

static char *zErrMsg      = NULL;
static sqlite *db         = NULL;

#define DATABASE "/etc/reg.db"

gboolean dbopen(void)
{
	if (!db) 
	{
		db = sqlite_open(DATABASE, 0, &zErrMsg);
		if (db == NULL)
			printf("error: %s\n", zErrMsg);
	}
	return db != NULL;
}

static int UpdateCallBack(void *NotUsed, int argc, char **argv, char **azColName)
{
	int *update = (int *) NotUsed;
	if (argv[0])
		*update = atoi(argv[0]);
	else
		*update = 0;
	return 0;
}

int getregnumbyname(const gchar *username)
{
	int number = -2;
	if (dbopen()) 
	{
		char SQL[512];
		sprintf(SQL, "SELECT number FROM user WHERE username ='%s'", username);	
		sqlite_exec(db, SQL, UpdateCallBack, &number, NULL);
	}
	return number;
}

int saveregcode(const char *username, const char *keycode, const char *regcode, int number)
{
	if (dbopen()) 
	{
		char SQL[512];
		int count = 0;
		sprintf(SQL, "SELECT COUNT(*) FROM regcode WHERE keycode='%s'", keycode);
		sqlite_exec(db, SQL, UpdateCallBack, &count, NULL);
		if (count == 0) 
		{
			time_t timep;
			struct tm *p;
			time(&timep);
			p = localtime(&timep); /*ȡ�õ���ʱ��*/

			sprintf(SQL, "INSERT INTO regcode VALUES('%s','%s','%s','%d-%d-%d %d:%d:%d');", 
					username, keycode, regcode, 
					p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
					p->tm_hour, p->tm_min, p->tm_sec);
			
			sqlite_exec(db, SQL, NULL, NULL, NULL);
			if (number > 0) 
			{
				sprintf(SQL, "UPDATE user SET number=number-1 WHERE username='%s'", username);
				sqlite_exec(db, SQL, NULL, NULL, NULL);
				number--;
			}
		}
	}
	return number;
}

static gchar *get_part_name(const gchar *username)
{
	const gchar *ch;

	g_return_val_if_fail (username != NULL, NULL);
	ch = strchr (username, '@');
	if (!ch)
		return NULL;

	return g_strndup (username, ch - username);
}

static void print_finger(const char *fpr, unsigned int  size)
{
	gint i;
	for (i = 0; i < size-1; i++)
		g_printerr ("%02X:", fpr[i]);
	
	g_printerr ("%02X", fpr[size-1]);
}

static LmSSLResponse ssl_cb (LmSSL *ssl, LmSSLStatus status, gpointer ud)
{
#if 0
	g_print ("GTalk: SSL status:%d\n", status);

	switch (status) {
	case LM_SSL_STATUS_NO_CERT_FOUND:
		g_printerr ("GTalk: No certificate found!\n");
		break;
	case LM_SSL_STATUS_UNTRUSTED_CERT:
		g_printerr ("GTalk: Certificate is not trusted!\n"); 
		break;
	case LM_SSL_STATUS_CERT_EXPIRED:
		g_printerr ("GTalk: Certificate has expired!\n"); 
		break;
	case LM_SSL_STATUS_CERT_NOT_ACTIVATED:
		g_printerr ("GTalk: Certificate has not been activated!\n"); 
		break;
	case LM_SSL_STATUS_CERT_HOSTNAME_MISMATCH:
		g_printerr ("GTalk: Certificate hostname does not match expected hostname!\n"); 
		break;
	case LM_SSL_STATUS_CERT_FINGERPRINT_MISMATCH: {
		const char *fpr = lm_ssl_get_fingerprint (ssl);
		g_printerr ("GTalk: Certificate fingerprint does not match expected fingerprint!\n"); 
		g_printerr ("GTalk: Remote fingerprint: ");
		print_finger (fpr, 16);

		g_printerr ("\nGTalk: Expected fingerprint: ");
		print_finger (expected_fingerprint, 16);
		g_printerr ("\n");
		break;
	}
	case LM_SSL_STATUS_GENERIC_ERROR:
		g_printerr ("GTalk: Generic SSL error!\n"); 
		break;
	}
#endif
	return LM_SSL_RESPONSE_CONTINUE;
}

static void connection_auth_cb (
		LmConnection *connection,
		gboolean      success, 
		gpointer      user_data)
{
	if (success) 
	{
		LmMessage *m;
		
		test_success = TRUE;

		m = lm_message_new_with_sub_type (NULL,
						LM_MESSAGE_TYPE_PRESENCE,
						LM_MESSAGE_SUB_TYPE_AVAILABLE);
		lm_connection_send (connection, m, NULL);

		lm_message_unref (m);
		g_print ("GTalk: Authenticated successfully\n");
	} else {
		g_printerr ("GTalk: Failed to authenticate\n");
		g_main_loop_quit(main_loop);
	}
}

static void connection_open_cb (
		LmConnection *connection, 
		gboolean      success,
		gpointer      user_data)
{
	if (success) {
		gchar *user;

		user = get_part_name (username);
		lm_connection_authenticate (connection, user, 
				password, resource,
				connection_auth_cb,
				NULL, FALSE,  NULL);
		g_free (user);
	} else {
		g_printerr ("GTalk: Failed to connect\n");
		g_main_loop_quit (main_loop);
	}
}

static void connection_close_cb (
		LmConnection       *connection, 
		LmDisconnectReason  reason,
		gpointer            user_data)
{
	const char *str;
	
	switch (reason) {
		case LM_DISCONNECT_REASON_OK:
			str = "LM_DISCONNECT_REASON_OK";
			break;
		case LM_DISCONNECT_REASON_PING_TIME_OUT:
			str = "LM_DISCONNECT_REASON_PING_TIME_OUT";
			break;
		case LM_DISCONNECT_REASON_HUP:
			str = "LM_DISCONNECT_REASON_HUP";
			break;
		case LM_DISCONNECT_REASON_ERROR:
			str = "LM_DISCONNECT_REASON_ERROR";
			break;
		case LM_DISCONNECT_REASON_UNKNOWN:
		default:
			str = "LM_DISCONNECT_REASON_UNKNOWN";
			break;
	}

	g_print("GTalk: Disconnected, reason:%d->'%s'\n", reason, str);
}

static char *parse_timestamp(const char *ts)
{
	char *time = g_strdup (strchr (ts, 'T') + 1);
	return time;
}

static gboolean SendMessage(
		LmConnection *connection,
		const char   *recipient, 
		const char   *message)
{
	LmMessage *m;
	GError    *error = NULL;
	gboolean   x = TRUE;

	m = lm_message_new (recipient, LM_MESSAGE_TYPE_MESSAGE);
	lm_message_node_add_child (m->node, "body", message);
	if (!lm_connection_send (connection, m, &error)) 
	{
		g_print ("Error while sending message to '%s':\n%s\n",
		recipient, error->message);
		x = FALSE;
	}
	lm_message_unref (m);
	return x;
}

static LmHandlerResult handle_messages (
		LmMessageHandler *handler,
		LmConnection  *connection,
		LmMessage     *msg,
		gpointer       user_data)
{
	LmMessageNode *root, *body, *x;
	const char *from, *msg_str, *type;
	char *ts = NULL;

	root = lm_message_get_node (msg);
	from = lm_message_node_get_attribute (msg->node, "from");
	body = lm_message_node_get_child (root, "body");
	if (body)
		msg_str = lm_message_node_get_value (body);

	type = lm_message_node_get_attribute (msg->node, "type");
	if (type && g_ascii_strcasecmp (type, "chat") != 0)
	{
		printf("[message of type '%s']", type);
		return LM_HANDLER_RESULT_REMOVE_MESSAGE;
	}
	for (x = root->children; x != NULL; x = x->next)
	{
		if (!g_ascii_strcasecmp (x->name, "x"))
		{
			const char *xmlns = lm_message_node_get_attribute (x, "xmlns");
			if (xmlns && !g_ascii_strcasecmp (xmlns, "jabber:x:delay"))
				ts = parse_timestamp ((char *)lm_message_node_get_attribute (x, "stamp"));
		}
	}
	if (strchr (from, '/'))
		*strchr (from, '/') = '\0';

	if (ts) g_free (ts);

//	printf("%s: %s\n", from, msg_str);
	int num = getregnumbyname(from);

	char sendmsg[200];
	if (num != -2) 
	{
		if (num == 0)
			strcpy(sendmsg, "您的注册点数已经用完，请重新申请点数.");
		else
		{
			if (strcasecmp(msg_str, "list") == 0)
			{

			}
			else {	
				char regkey[33];
				if (CreateRegCode(msg_str, regkey)) {
					num = saveregcode(from, msg_str, regkey, num);
					if (num >= 0)
						sprintf(sendmsg, "%s->%s，您还可以注册次数: %d", msg_str, regkey, num);
					else
						sprintf(sendmsg, "%s->%s", msg_str, regkey);
					char selfmsg[512];
					sprintf(selfmsg, "%s: %s->%s\n", from, msg_str, regkey);
					SendMessage(connection, "chinaktv@gmail.com", selfmsg);		
				} else
					sprintf(sendmsg, "您的认证码 \"%s\" 有错误, 请检查您的认证码是否正确。", msg_str);
			}
		}
	} else
		strcpy(sendmsg, "您不是注册用户，不能申请注册码。");
		
	SendMessage(connection, from, sendmsg);
	return LM_HANDLER_RESULT_REMOVE_MESSAGE;
}

static void init_daemon(void)
{
	int pid;
	int i;
	if (pid = fork())
		exit(0); // end parent
	else if (pid < 0)
		exit(1);
	setsid();
	if (pid=fork())
		exit(0);
	else if (pid<0)
		exit(1);
	for (i=0;i<NOFILE;i++)
		close(i);
	chdir("/tmp");
	umask(0);
	return;
}

int main (int argc, char **argv)
{
	init_daemon();
	LmConnection     *connection;
	LmMessageHandler *handler;
	gboolean          result;
	GError           *error = NULL;

	connection = lm_connection_new (server);
	lm_connection_set_port (connection, port);
	lm_connection_set_jid (connection, username);

	handler = lm_message_handler_new (handle_messages, NULL, NULL);
	lm_connection_register_message_handler (connection, handler, 
		LM_MESSAGE_TYPE_MESSAGE, 
		LM_HANDLER_PRIORITY_NORMAL);
	
	lm_message_handler_unref (handler);
	
	lm_connection_set_disconnect_function (connection,
			connection_close_cb, NULL, NULL);

	if (fingerprint)
	{
		LmSSL *ssl;
		char  *p;
		int    i;
		
		if (port == LM_CONNECTION_DEFAULT_PORT)
			lm_connection_set_port (connection, LM_CONNECTION_DEFAULT_PORT_SSL);

		for (i = 0, p = fingerprint; *p && *(p+1); i++, p += 3)
			expected_fingerprint[i] = (unsigned char) g_ascii_strtoull (p, NULL, 16);
	
		ssl = lm_ssl_new (expected_fingerprint,
			(LmSSLFunction) ssl_cb, NULL, NULL);
	
		lm_connection_set_ssl (connection, ssl);
		lm_ssl_unref (ssl);
	}

	result = lm_connection_open (connection,
			(LmResultFunction) connection_open_cb, NULL, NULL, &error);

	if (!result) {
		g_printerr ("GTalk: Opening connection failed, error:%d->'%s'\n", 
			error->code, error->message);
		g_free (error);
		return EXIT_FAILURE;
	}

	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);
	return (test_success ? EXIT_SUCCESS : EXIT_FAILURE);
}

