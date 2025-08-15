/**
 * Test program for Ollama AI fallback integration
 * Compile: gcc -o test_ollama test_ollama_ai.c -lcurl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define OLLAMA_API_ENDPOINT "http://localhost:11434/api/generate"
#define OLLAMA_MODEL "llama3.2:1b"

struct curl_response {
  char *data;
  size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct curl_response *mem = (struct curl_response *)userp;
  
  char *ptr = realloc(mem->data, mem->size + realsize + 1);
  if (!ptr) {
    return 0;
  }
  
  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->data[mem->size] = '\0';
  
  return realsize;
}

static char *parse_ollama_response(const char *json_str) {
  const char *response_start, *response_end;
  char *result;
  size_t response_len;
  int i, j;
  
  /* Find "response":" in the JSON */
  response_start = strstr(json_str, "\"response\":\"");
  if (!response_start) {
    return NULL;
  }
  
  response_start += 12; /* Skip past "response":" */
  
  /* Find the closing quote */
  response_end = response_start;
  while (*response_end) {
    if (*response_end == '"' && *(response_end - 1) != '\\') {
      break;
    }
    response_end++;
  }
  
  if (!*response_end) {
    return NULL;
  }
  
  response_len = response_end - response_start;
  result = malloc(response_len + 1);
  if (!result) {
    return NULL;
  }
  
  /* Copy and unescape the response */
  for (i = 0, j = 0; i < (int)response_len; i++) {
    if (response_start[i] == '\\' && i + 1 < (int)response_len) {
      switch (response_start[i + 1]) {
        case 'n': result[j++] = '\n'; i++; break;
        case 'r': result[j++] = '\r'; i++; break;
        case 't': result[j++] = '\t'; i++; break;
        case '"': result[j++] = '"'; i++; break;
        case '\\': result[j++] = '\\'; i++; break;
        default: result[j++] = response_start[i]; break;
      }
    } else {
      result[j++] = response_start[i];
    }
  }
  result[j] = '\0';
  
  return result;
}

int main(int argc, char *argv[]) {
  CURL *curl;
  CURLcode res;
  struct curl_response response = {0};
  struct curl_slist *headers = NULL;
  char json_request[4096];
  char *ollama_response;
  const char *prompt;
  
  if (argc > 1) {
    prompt = argv[1];
  } else {
    prompt = "A player greets you warmly";
  }
  
  printf("Testing Ollama AI integration...\n");
  printf("Prompt: %s\n", prompt);
  printf("Endpoint: %s\n", OLLAMA_API_ENDPOINT);
  printf("Model: %s\n\n", OLLAMA_MODEL);
  
  /* Build JSON request */
  snprintf(json_request, sizeof(json_request),
    "{"
      "\"model\":\"%s\","
      "\"prompt\":\"You are an NPC in a fantasy RPG game. %s Respond in character briefly:\","
      "\"stream\":false,"
      "\"options\":{"
        "\"num_predict\":30,"
        "\"temperature\":0.7,"
        "\"top_k\":40,"
        "\"top_p\":0.9"
      "}"
    "}",
    OLLAMA_MODEL,
    prompt
  );
  
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  
  if (!curl) {
    fprintf(stderr, "Failed to initialize CURL\n");
    return 1;
  }
  
  /* Set headers */
  headers = curl_slist_append(headers, "Content-Type: application/json");
  
  /* Configure CURL */
  curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_API_ENDPOINT);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);
  
  printf("Sending request to Ollama...\n");
  res = curl_easy_perform(curl);
  
  if (res == CURLE_OK) {
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (http_code == 200) {
      printf("Success! HTTP 200\n");
      printf("Raw response (first 200 chars): %.200s...\n\n", response.data);
      
      ollama_response = parse_ollama_response(response.data);
      if (ollama_response) {
        printf("NPC Response: %s\n", ollama_response);
        free(ollama_response);
      } else {
        printf("Failed to parse response\n");
      }
    } else {
      printf("HTTP error: %ld\n", http_code);
      if (response.data) {
        printf("Error response: %s\n", response.data);
      }
    }
  } else {
    printf("CURL error: %s\n", curl_easy_strerror(res));
    printf("Is Ollama running? Check with: systemctl status ollama\n");
  }
  
  /* Cleanup */
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  if (response.data) {
    free(response.data);
  }
  curl_global_cleanup();
  
  return 0;
}