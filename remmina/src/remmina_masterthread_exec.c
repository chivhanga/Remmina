/*  See LICENSE and COPYING files for copyright and license details. */

/* Support for execution on main thread of some GTK related
 * functions (due to threads deprecations in GTK) */

#include <gtk/gtk.h>



#include "remmina_masterthread_exec.h"


static pthread_t gMainThreadID;

static gboolean remmina_masterthread_exec_callback(RemminaMTExecData *d)
{

	/* This function is called on main GTK Thread via gdk_threads_add_idlde()
	 * from remmina_masterthread_exec_and_wait() */

	if (!d->cancelled) {
		switch(d->func) {
			case FUNC_INIT_SAVE_CRED:
				remmina_protocol_widget_init_save_cred(d->p.init_save_creds.gp);
				break;
			case FUNC_CHAT_RECEIVE:
				remmina_protocol_widget_chat_receive(d->p.chat_receive.gp, d->p.chat_receive.text);
				break;
			case FUNC_FILE_GET_SECRET:
				d->p.file_get_secret.retval = remmina_file_get_secret( d->p.file_get_secret.remminafile, d->p.file_get_secret.setting );
				break;
			case FUNC_DIALOG_SERVERKEY_CONFIRM:
				d->p.dialog_serverkey_confirm.retval = remmina_init_dialog_serverkey_confirm( d->p.dialog_serverkey_confirm.dialog,
					d->p.dialog_serverkey_confirm.serverkey, d->p.dialog_serverkey_confirm.prompt );
				break;
			case FUNC_DIALOG_AUTHPWD:
				d->p.dialog_authpwd.retval = remmina_init_dialog_authpwd(d->p.dialog_authpwd.dialog,
					d->p.dialog_authpwd.label, d->p.dialog_authpwd.allow_save);
				break;
			case FUNC_GTK_LABEL_SET_TEXT:
				gtk_label_set_text( d->p.gtk_label_set_text.label, d->p.gtk_label_set_text.str );
				break;
			case FUNC_DIALOG_AUTHUSERPWD:
				d->p.dialog_authuserpwd.retval = remmina_init_dialog_authuserpwd( d->p.dialog_authuserpwd.dialog,
					d->p.dialog_authuserpwd.want_domain, d->p.dialog_authuserpwd.default_username,
					d->p.dialog_authuserpwd.default_domain, d->p.dialog_authuserpwd.allow_save );
				break;
			case FUNC_DIALOG_CERT:
				d->p.dialog_certificate.retval = remmina_init_dialog_certificate( d->p.dialog_certificate.dialog,
					d->p.dialog_certificate.subject, d->p.dialog_certificate.issuer, d->p.dialog_certificate.fingerprint );
				break;
			case FUNC_DIALOG_CERTCHANGED:
				d->p.dialog_certchanged.retval = remmina_init_dialog_certificate_changed( d->p.dialog_certchanged.dialog,
					d->p.dialog_certchanged.subject, d->p.dialog_certchanged.issuer, d->p.dialog_certchanged.new_fingerprint,
					d->p.dialog_certchanged.old_fingerprint );
				break;
			case FUNC_DIALOG_AUTHX509:
				d->p.dialog_authx509.retval = remmina_init_dialog_authx509( d->p.dialog_authx509.dialog, d->p.dialog_authx509.cacert,
					d->p.dialog_authx509.cacrl, d->p.dialog_authx509.clientcert, d->p.dialog_authx509.clientkey );
				break;
			case FUNC_FTP_CLIENT_UPDATE_TASK:
				remmina_ftp_client_update_task( d->p.ftp_client_update_task.client, d->p.ftp_client_update_task.task );
				break;
			case FUNC_FTP_CLIENT_GET_WAITING_TASK:
				d->p.ftp_client_get_waiting_task.retval = remmina_ftp_client_get_waiting_task( d->p.ftp_client_get_waiting_task.client );
				break;
			case FUNC_SFTP_CLIENT_CONFIRM_RESUME:
#ifdef HAVE_LIBSSH
				d->p.sftp_client_confirm_resume.retval = remmina_sftp_client_confirm_resume( d->p.sftp_client_confirm_resume.client,
					d->p.sftp_client_confirm_resume.path );
#endif
				break;
			case FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY:
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
				remmina_plugin_ssh_vte_terminal_set_encoding_and_pty( d->p.vte_terminal_set_encoding_and_pty.terminal,
					d->p.vte_terminal_set_encoding_and_pty.codeset, d->p.vte_terminal_set_encoding_and_pty.slave );
#endif
				break;
		}
		pthread_mutex_unlock(&d->mu);
	} else {
		/* thread has been cancelled, so we must free d memory here */
		g_free(d);
	}
	return G_SOURCE_REMOVE;
}

static void remmina_masterthread_exec_cleanup_handler(RemminaMTExecData *d)
{
	d->cancelled = TRUE;
}

void remmina_masterthread_exec_and_wait(RemminaMTExecData *d)
{
	d->cancelled = FALSE;
	pthread_cleanup_push(remmina_masterthread_exec_cleanup_handler, (void *)d);
	pthread_mutex_init(&d->mu, NULL);
	pthread_mutex_lock(&d->mu);
	gdk_threads_add_idle((GSourceFunc)remmina_masterthread_exec_callback, (gpointer)d);
	pthread_mutex_lock(&d->mu);
	pthread_cleanup_pop(0);
	pthread_mutex_unlock(&d->mu);
	pthread_mutex_destroy(&d->mu);
}

void remmina_masterthread_exec_save_main_thread_id() {
	/* To be called from main thread at startup */
	gMainThreadID = pthread_self();
}

gboolean remmina_masterthread_exec_is_main_thread() {
	return pthread_equal(gMainThreadID, pthread_self()) != 0;
}
