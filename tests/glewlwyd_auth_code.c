/* Public domain, no copyright. Use at your own risk. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <check.h>
#include <ulfius.h>
#include <orcania.h>
#include <yder.h>

#include "unit-tests.h"

#define SERVER_URI "http://localhost:4593/glewlwyd"
#define USERNAME "user1"
#define PASSWORD "MyUser1Password!"
#define SCOPE_LIST "scope1 scope2"
#define CLIENT "client1_id"

struct _u_request user_req;

START_TEST(test_glwd_auth_invalid_response_type)
{
  char * url = msprintf("%s/auth?response_type=invalid&client_id=client1_id&redirect_uri=../static/index.html&state=xyzabcd&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(NULL, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "unsupported_response_type");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_state_ok)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client1_id&redirect_uri=../static/index.html&state=xyzabcd&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(NULL, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "state=xyzabcd");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_ok_redirect_login)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client1_id&redirect_uri=../static/index.html?param=client1_cb1&state=xyz&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(NULL, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "login.html");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_client_invalid)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client_error&redirect_uri=../static/index.html?param=client1_cb1&state=xyz&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(NULL, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "unauthorized_client");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_uri_invalid)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client_error&redirect_uri=../static/index.html?param=invalid&state=xyz&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(NULL, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "unauthorized_client");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_scope_invalid)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client1_id&redirect_uri=../static/index.html?param=client1_cb1&state=xyzabcd&scope=scope4&login_validated=true", SERVER_URI);
  int res = run_simple_test(&user_req, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "invalid_scope");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_scope_empty)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client1_id&redirect_uri=../static/index.html?param=client1_cb1&state=xyzabcd&login_validated=true", SERVER_URI);
  int res = run_simple_test(&user_req, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "invalid_scope");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

START_TEST(test_glwd_auth_code_ok_redirect_cb_with_code)
{
  char * url = msprintf("%s/auth?response_type=code&client_id=client1_id&redirect_uri=../static/index.html?param=client1_cb1&state=xyzabcd&scope=scope1 scope2&login_validated=true", SERVER_URI);
  int res = run_simple_test(&user_req, "GET", url, NULL, NULL, NULL, NULL, 302, NULL, NULL, "code=");
  free(url);
	ck_assert_int_eq(res, 1);
}
END_TEST

static Suite *libjwt_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("Glewlwyd auth_code");
	tc_core = tcase_create("test_glwd_auth_code");
	tcase_add_test(tc_core, test_glwd_auth_invalid_response_type);
	tcase_add_test(tc_core, test_glwd_auth_code_ok_redirect_login);
	tcase_add_test(tc_core, test_glwd_auth_code_state_ok);
	tcase_add_test(tc_core, test_glwd_auth_code_client_invalid);
	tcase_add_test(tc_core, test_glwd_auth_code_uri_invalid);
	tcase_add_test(tc_core, test_glwd_auth_code_scope_invalid);
	tcase_add_test(tc_core, test_glwd_auth_code_scope_empty);
	tcase_add_test(tc_core, test_glwd_auth_code_ok_redirect_cb_with_code);
	tcase_set_timeout(tc_core, 30);
	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char *argv[])
{
	int number_failed;
	Suite *s;
	SRunner *sr;
  struct _u_request auth_req, scope_req;
  struct _u_response auth_resp, scope_resp;
  int res;
  
  y_init_logs("Glewlwyd test", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting Glewlwyd test");
  
  // Getting a valid session id for authenticated http requests
  ulfius_init_request(&auth_req);
  ulfius_init_request(&user_req);
  ulfius_init_response(&auth_resp);
  auth_req.http_verb = strdup("POST");
  auth_req.http_url = msprintf("%s/user/auth", SERVER_URI);
  u_map_put(auth_req.map_post_body, "username", USERNAME);
  u_map_put(auth_req.map_post_body, "password", PASSWORD);
  res = ulfius_send_http_request(&auth_req, &auth_resp);
  if (res == U_OK) {
    y_log_message(Y_LOG_LEVEL_INFO, "User %s authenticated", USERNAME);
    int i;
    for (i=0; i<auth_resp.nb_cookies; i++) {
      char * cookie = msprintf("%s=%s", auth_resp.map_cookie[i].key, auth_resp.map_cookie[i].value);
      u_map_put(user_req.map_header, "Cookie", cookie);
      free(cookie);
    }
    
    ulfius_init_request(&scope_req);
    ulfius_init_response(&scope_resp);
    scope_req.http_verb = strdup("POST");
    scope_req.http_url = msprintf("%s/user/grant", SERVER_URI);
    u_map_put(scope_req.map_post_body, "scope", SCOPE_LIST);
    u_map_put(scope_req.map_post_body, "client_id", CLIENT);
    if (ulfius_send_http_request(&auth_req, &auth_resp) != U_OK) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "Grant scope %s for %s error", CLIENT, SCOPE_LIST);
    }
    ulfius_clean_request(&scope_req);
    ulfius_clean_response(&scope_resp);
  }
  ulfius_clean_request(&auth_req);
  ulfius_clean_response(&auth_resp);
  
	s = libjwt_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
  
  ulfius_clean_request(&user_req);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}