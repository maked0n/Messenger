#include "MessengerBackend.h"

CMessengerBackend::CMessengerBackend(const std::string& server_url, unsigned short port) {
	messenger::MessengerSettings msg_settings;
	CMessengerBackend::_set_settings(server_url, port, msg_settings);
	m_messenger_instance = messenger::GetMessengerInstance(msg_settings);
	m_message_observer = new CMessageObserver;
	m_login_callback = new callbacks::CLoginCallback(m_messenger_instance, m_message_observer);
	m_request_user_callback = new callbacks::CRequestUserCallback(&m_online_users);
	m_cur_user = 0;
	m_use_encryption = false;
	m_cryptographer = NULL;
}

CMessengerBackend::~CMessengerBackend() {
	if (m_login_callback) delete m_login_callback;
	if (m_request_user_callback) delete m_request_user_callback;
	if (m_message_observer) delete m_message_observer;
	if (m_cryptographer) delete m_cryptographer;
}

void CMessengerBackend::_set_settings(const std::string& server_url, unsigned short port, messenger::MessengerSettings& msg_settings) {
	msg_settings.serverUrl = server_url;
	msg_settings.serverPort = port;
}

void CMessengerBackend::_set_policy(bool use_encryption, messenger::SecurityPolicy& sec_policy) {
	if (use_encryption) {
		sec_policy.encryptionAlgo = messenger::encryption_algorithm::RSA_1024;
		//m_cryptographer->get_pub_key(sec_policy.encryptionPubKey);
		//TODO: set public key
	}
	else sec_policy.encryptionAlgo = messenger::encryption_algorithm::None;
}

void CMessengerBackend::login(const std::string& user_id, const std::string& password, bool use_encryption, callbacks::pManagedCallback callback_func) {
	messenger::SecurityPolicy sec_policy;
	m_use_encryption = use_encryption;
	CMessengerBackend::_set_policy(use_encryption, sec_policy);
	if (use_encryption) {
		m_cryptographer = new CCryptographer;
		m_cryptographer->generate_key_pair();
	}
	m_login_callback->set_callback(callback_func);
	m_message_observer->set_user(user_id);
	m_messenger_instance->Login(user_id, password, sec_policy, m_login_callback);
}

void CMessengerBackend::disconnect() {
	m_messenger_instance->UnregisterObserver(m_message_observer);
	m_messenger_instance->Disconnect();
}

void CMessengerBackend::send_message(const std::string& user_id, unsigned char* data, int data_size, messenger::message_content_type::Type type) {
	messenger::MessageContent content;
	content.encrypted = m_use_encryption;
	content.type = type;
	if (m_use_encryption) m_cryptographer->encrypt_message(data, data_size, content.data);
	else for (int i = 0; i < data_size; ++i) content.data.push_back(data[i]);
	m_cur_message = m_messenger_instance->SendMessage(user_id, content);
}

void CMessengerBackend::send_message_seen(const std::string& user_id, const std::string& message_id) {
	m_messenger_instance->SendMessageSeen(user_id, message_id);
}

void CMessengerBackend::request_active_users(callbacks::pManagedCallback callback_func) {
	m_request_user_callback->set_callback(callback_func);
	m_messenger_instance->RequestActiveUsers(m_request_user_callback);
}

const char* CMessengerBackend::get_next_user(int* str_len) {
	if (m_cur_user < m_online_users.size()) {
		*str_len = m_online_users.at(m_cur_user).identifier.length();
		const char* res = m_online_users.at(m_cur_user).identifier.c_str();
		++m_cur_user;
		return res;
	}
	else {
		m_cur_user = 0;
		*str_len = 0;
		return NULL;
	}
}

const char* CMessengerBackend::get_last_msg_id(int* str_len) {
	*str_len = m_cur_message.identifier.length();
	return m_cur_message.identifier.c_str();
}

std::time_t CMessengerBackend::get_last_msg_date() {
	return m_cur_message.time;
}

void CMessengerBackend::set_msg_status_changed_callback(pMessageStatusChanged callback_func) {
	m_message_observer->set_status_changed_callback(callback_func);
}

void CMessengerBackend::set_msg_received_callback(pMessageReceived callback_func) {
	m_message_observer->set_received_callback(callback_func);
}

extern "C" __declspec(dllexport) CMessengerBackend* _stdcall create_backend_instance(char* server_url, unsigned short port) {
	return new CMessengerBackend(std::string(server_url), port);
}

extern "C" __declspec(dllexport) void _stdcall dispose_class(CMessengerBackend* pObject) {
	if (pObject != NULL) {
		delete pObject;
		pObject = NULL;
	}
}

extern "C" __declspec(dllexport) void _stdcall call_login(CMessengerBackend* pObject, char* user_id, char* password, 
														bool use_encryption, callbacks::pManagedCallback callback_func) {
	if (pObject != NULL) {
		std::string s_user_id(user_id);
		std::string s_password(password);
		pObject->login(s_user_id, s_password, use_encryption, callback_func);
	}
}

extern "C" __declspec(dllexport) void _stdcall call_disconnect(CMessengerBackend* pObject) {
	if (pObject != NULL)
		pObject->disconnect();
}

/*
type = 1	text
type = 2	image
type = 3	video
*/
extern "C" __declspec(dllexport) void _stdcall call_send_message(CMessengerBackend* pObject, char* user_id,
															   unsigned char* data, int data_size, int type) {
	std::string s_user_id(user_id);
	messenger::message_content_type::Type msg_type;
	switch (type) {
	case 1:
		msg_type = messenger::message_content_type::Text;
		break;
	case 2:
		msg_type = messenger::message_content_type::Image;
		break;
	case 3:
		msg_type = messenger::message_content_type::Video;
		break;
	default:
		msg_type = messenger::message_content_type::Text;
	}
	pObject->send_message(s_user_id, data, data_size, msg_type);
}

extern "C" __declspec(dllexport) const char* _stdcall get_last_msg_id(CMessengerBackend* pObject, int* str_len) {
	const char* res = pObject->get_last_msg_id(str_len);
	return res;
}

extern "C" __declspec(dllexport) long int _stdcall get_last_msg_time(CMessengerBackend* pObject) {
	std::time_t res = pObject->get_last_msg_date();
	return static_cast<long int>(res);
}

extern "C" _declspec(dllexport) const char* _stdcall get_next_user(CMessengerBackend* pObject, int* str_len) {
	return pObject->get_next_user(str_len);
}

extern "C" _declspec(dllexport) void _stdcall free_user_list(const char** arr, int size) {
	for (int i = 0; i < size; ++i)
		delete arr[i];
	delete arr;
}

extern "C" __declspec(dllexport) void _stdcall call_send_message_seen(CMessengerBackend* pObject, char* user_id, char* message_id) {
	std::string s_user_id(user_id);
	std::string s_message_id(message_id);
	pObject->send_message_seen(s_user_id, s_message_id);
}

extern "C" __declspec(dllexport) void _stdcall call_request_active_users(CMessengerBackend* pObject, callbacks::pManagedCallback callback_func) {
	pObject->request_active_users(callback_func);
}

extern "C" __declspec(dllexport) void _stdcall set_msg_status_changed_callback(CMessengerBackend* pObject, pMessageStatusChanged callback_func) {
	pObject->set_msg_status_changed_callback(callback_func);
}

extern "C" __declspec(dllexport) void _stdcall set_msg_received_callback(CMessengerBackend* pObject, pMessageReceived callback_func) {
	pObject->set_msg_received_callback(callback_func);
}

extern "C" __declspec(dllexport) void _stdcall free_data(unsigned char* data) {
	if (data != NULL) {
		delete data;
		data = NULL;
	}
}