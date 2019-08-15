/*
 * Copyright © 2016 Jorgen Lind
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "json_struct.h"

#include "assert.h"

const char json[] =
"{\n"
"    \"property_one\" : 432432,\n"
"    \"execute_one\" : {\n"
"        \"number\" : 45,\n"
"        \"valid\" : \"false\"\n"
"    },"
"    \"execute_two\" : 99,\n"
"    \"execute_three\" : [\n"
"        4,\n"
"        6,\n"
"        8\n"
"    ]\n"
"}\n";

struct SubObject
{
    int number;
    bool valid;

    JS_OBJECT(JS_MEMBER(number),
              JS_MEMBER(valid));
};

void js_validate_json(JS::Tokenizer &tokenizer)
{
    JS::Token token;;
    JS::Error error;
    std::string buffer;
    
    error = tokenizer.nextToken(token);
    JS_ASSERT(error == JS::Error::NoError);
    JS_ASSERT(token.value_type == JS::Type::ObjectStart);
    
    error = tokenizer.nextToken(token);
    JS_ASSERT(error == JS::Error::NoError);
    JS_ASSERT(token.value_type == JS::Type::Number);
    
    error = tokenizer.nextToken(token);
    JS_ASSERT(error == JS::Error::NoError);
    JS_ASSERT(token.value_type == JS::Type::ObjectStart);
    tokenizer.copyFromValue(token, buffer);
    
    while(error == JS::Error::NoError && token.value_type != JS::Type::ObjectEnd)
        error = tokenizer.nextToken(token);
    
    JS_ASSERT(error == JS::Error::NoError);
    JS_ASSERT(token.value_type == JS::Type::ObjectEnd);
    tokenizer.copyIncludingValue(token, buffer);
    
    while(error == JS::Error::NoError && token.value_type != JS::Type::ObjectEnd)
        error = tokenizer.nextToken(token);
    
    JS::ParseContext context(buffer.c_str(), buffer.size());
    fprintf(stderr, "buffer %s\n", buffer.c_str());
    SubObject subObj;
    context.parseTo(subObj);
    
    JS_ASSERT(context.error == JS::Error::NoError);
    JS_ASSERT(subObj.number == 45);
    JS_ASSERT(subObj.valid == false);
}
void js_copy_full()
{
    JS::Tokenizer tokenizer;
    tokenizer.addData(json);
    js_validate_json(tokenizer);
}

void js_partial_1()
{
    JS::Tokenizer tokenizer;
    tokenizer.addData(json,40);
    tokenizer.addData(json + 40, sizeof(json) - 40);
    js_validate_json(tokenizer);
}

void js_partial_2()
{
    JS::Tokenizer tokenizer;
    size_t offset = 0;
    std::function<void (JS::Tokenizer &)> func = [&offset, &func] (JS::Tokenizer &tok)
    {
        if (offset + 2 > sizeof(json)) {
            tok.addData(json + offset, sizeof(json) - offset);
            offset += sizeof(json) - offset;
        } else {
            tok.addData(json + offset, 2);
            offset += 2;
        }
    };
    auto ref = tokenizer.registerNeedMoreDataCallback(func);

    js_validate_json(tokenizer);
}

void js_partial_3()
{
    JS::Tokenizer tokenizer;
    size_t offset = 0;
    std::function<void (JS::Tokenizer &)> func = [&offset, &func] (JS::Tokenizer &tokenizer)
    {
        if (offset + 1 > sizeof(json)) {
            tokenizer.addData(json + offset, sizeof(json) - offset);
            offset += sizeof(json) - offset;
        } else {
            tokenizer.addData(json + offset, 1);
            offset += 1;
        }
    };
    auto ref = tokenizer.registerNeedMoreDataCallback(func);
    
    js_validate_json(tokenizer);
}

const char json2[] =
R"json({
	"test" : true,
	"more" : {
		"sub_object_prop1" : true,
		"sub_object_prop2" : 456
	},
	"int_value" : 65
})json";

struct Child
{
	bool sub_object_prop1;
	int sub_object_prop2;
	JS_OBJECT(JS_MEMBER(sub_object_prop1),
			  JS_MEMBER(sub_object_prop2));
};

struct Parent
{
	bool test;
	Child more;
	int int_value;
	JS_OBJECT(JS_MEMBER(test),
		      JS_MEMBER(more),
		      JS_MEMBER(int_value));
};

void js_copy_parsed()
{
	JS::Tokenizer tokenizer;
	tokenizer.addData(json2);

	JS::Token token;;
	JS::Error error = JS::Error::NoError;
	std::vector<JS::Token> tokens;
	while (error == JS::Error::NoError) {
		error = tokenizer.nextToken(token);
		tokens.push_back(token);
	}

	JS::ParseContext context;
	context.tokenizer.addData(&tokens);
	Parent parent;
	context.parseTo(parent);

	JS_ASSERT(context.error == JS::Error::NoError);
	JS_ASSERT(parent.test == true);
	JS_ASSERT(parent.more.sub_object_prop1 == true);
	JS_ASSERT(parent.more.sub_object_prop2 == 456);
	JS_ASSERT(parent.int_value == 65);
}

const char json_token_copy[] = R"json(
{
    "number" : 45,
    "valid" : false,
    "child" : {
        "some_more": "world",
        "another_int" : 495
    },
    "more_data" : "string data",
    "super_data" : "hello"
}
)json";

struct SecondChild
{
    std::string some_more;
    int another_int;
    JS_OBJECT(JS_MEMBER(some_more),
        JS_MEMBER(another_int));
};
struct SecondParent
{
    int number;
    bool valid;
    JS::JsonTokens child;
    std::string more_data;
    std::string super_data;

    JS_OBJECT(JS_MEMBER(number),
        JS_MEMBER(valid),
        JS_MEMBER(child),
        JS_MEMBER(more_data),
        JS_MEMBER(super_data));
};

void js_copy_tokens()
{
    SecondParent parent;
    JS::ParseContext parseContext(json_token_copy);
    parseContext.parseTo(parent);

    JS_ASSERT(parseContext.error == JS::Error::NoError);
    JS_ASSERT(parent.child.data.size() == 4);

    JS::ParseContext childContext;
    childContext.tokenizer.addData(&parent.child.data);
    SecondChild child;
    childContext.parseTo(child);
    JS_ASSERT(child.another_int == 495);
    JS_ASSERT(child.some_more == "world");
     
}
int main()
{
    js_copy_full();
    js_partial_1();
    js_partial_2();
    js_partial_3();

	js_copy_parsed();
    js_copy_tokens();
    return 0;
}
