#include <fstream>
#include <curl/curl.h>
#include <vector>
#include <tgbot/tgbot.h>
#include "./include/Photo_handler.h"

using namespace std;
using namespace TgBot;

//Path
string pathIn = "C:\\Users\\Max\\source\\repos\\DecriptionApp\\";

//Bot text
string start_text = "Hi, I'm a bot which can embed or extract messages to/from photos, I use the Quantization Index Modulation (QMI) method for this action. Choose, what do you want to do?";
// Embedding
string ask_quantization_embed = "Great, enter the quantization step for embed.";
string ask_photo_embed = "Great, send photo which you want to embed your text.";
string ask_open_text = "Great, enter open text for embed.";
// Extraction
string ask_photo_extract = "Great, send photo which you want to extract your text. (Bot supported only Png format for extraction)";
string ask_quantization_extract = "Great, enter the quantization step for extract.";

//Global ptr for delete messages
CallbackQuery::Ptr prev_callback_ptr;
Message::Ptr prev_bot_message;
Message::Ptr prev_person_message;

//Init all functions
size_t write_callback_helper(void* contents, size_t size, size_t nmemb, void* userp);
int save_photo_from_tg(string token, string file_path, string file_name);
std::vector<BotCommand::Ptr> create_commands();
InlineKeyboardMarkup::Ptr create_keyboard();
InlineKeyboardMarkup::Ptr create_cancel_button();
string embed_handler_photo(Message::Ptr message, Bot& bot);
void start_bot(Message::Ptr message, Bot& bot, InlineKeyboardMarkup::Ptr keyboard);
void rerun(Bot& bot);
void any_message_handler(Message::Ptr message, CallbackQuery::Ptr query, Bot& bot, InlineKeyboardMarkup::Ptr cancel_btn, string* ans_photo, string* ans_open_text, int* ans_quantization);


int main() {
    //Setlocale Ru lang
    setlocale(LC_ALL, "Russian");
    
    //Init bot
    string token("6124064658:AAHzxN0DzEAXwrmtt34GsDNxQic4IR-eIE0");
    printf("Token: %s\n", token.c_str());
    Bot bot(token);
    bot.getApi().setMyCommands(create_commands());

    //Inline Keyboard Markup
    InlineKeyboardMarkup::Ptr keyboard = create_keyboard();
    InlineKeyboardMarkup::Ptr cancel_btn = create_cancel_button();

    //Commands
    bot.getEvents().onCommand("start", [&bot, &keyboard](Message::Ptr message) {
        rerun(bot);
        start_bot(message, bot, keyboard);
    });

    bot.getEvents().onCommand("rerun", [&bot, &keyboard](Message::Ptr message) {
        rerun(bot);
        start_bot(message, bot, keyboard);
    });


    //Callback handler 
    string ans_photo;
    int ans_quantization = 0;
    string ans_open_text;

    bot.getEvents().onCallbackQuery([&bot, &cancel_btn, &ans_photo, &ans_quantization, &ans_open_text, &keyboard](CallbackQuery::Ptr query) {

        //Embed    
        if (StringTools::startsWith(query->data, "embed") && ans_photo.empty() && ans_open_text.empty() && !ans_quantization) {
            prev_callback_ptr = query;
            //Send message
            prev_bot_message = bot.getApi().sendMessage(query->message->chat->id, ask_photo_embed, false, 0, cancel_btn);
            bot.getEvents().onAnyMessage([query, &bot, &cancel_btn, &ans_photo, &ans_open_text, &ans_quantization](Message::Ptr message) {
                any_message_handler(message, query, bot, cancel_btn, &ans_photo, &ans_open_text, &ans_quantization);
                });
        }

        //Extract
        if (StringTools::startsWith(query->data, "extract")) {
            prev_callback_ptr = query;
            //Send message
            prev_bot_message = bot.getApi().sendMessage(query->message->chat->id, ask_photo_extract, false, 0, cancel_btn);
            bot.getEvents().onAnyMessage([query, &bot, &cancel_btn, &ans_photo, &ans_open_text, &ans_quantization](Message::Ptr message) {
                any_message_handler(message, query, bot, cancel_btn, &ans_photo, &ans_open_text, &ans_quantization);
            });
        }

        //Cancel
        if (StringTools::startsWith(query->data, "cancel")) {
            rerun(bot);

            ans_photo.clear();
            ans_open_text.clear();
            ans_quantization = 0;
            bot.getApi().sendMessage(query->message->chat->id, start_text, false, 0, keyboard);
            return;
        }
    });


    //Bot start 
    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (exception& e) {
        //Error handler
        printf("error: %s\n", e.what());
    }

    return 0;
}

//Bot Functions
void start_bot(Message::Ptr message, Bot& bot, InlineKeyboardMarkup::Ptr keyboard) {
    bot.getApi().sendMessage(message->chat->id, "Hi, I'm a bot that can embed or extract messages from photos using the Quantization Index Modulation (QMI) method. Choose, what you want to do?", false, 0, keyboard);
}
std::vector<BotCommand::Ptr> create_commands() {
    vector<BotCommand::Ptr> commands;
    //Start
    BotCommand::Ptr cmdArray1(new BotCommand);
    BotCommand::Ptr cmdArray2(new BotCommand);
    cmdArray2->command = "start";
    cmdArray2->description = "Start bot";

    cmdArray1->command = "rerun";
    cmdArray1->description = "Run bot again";

    commands.push_back(cmdArray2);
    commands.push_back(cmdArray1);

    return commands;
}
void rerun(Bot& bot) {
    if (prev_callback_ptr) {
        bot.getApi().deleteMessage(prev_callback_ptr->message->chat->id, prev_callback_ptr->message->messageId);
        prev_callback_ptr->data.clear();
    }

    if (prev_bot_message)
        bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
    if (prev_person_message)
        bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);

    prev_person_message = nullptr;
    prev_bot_message = nullptr;
    prev_callback_ptr = nullptr;
}
void any_message_handler(Message::Ptr message, CallbackQuery::Ptr query, Bot& bot, InlineKeyboardMarkup::Ptr cancel_btn, string* ans_photo, string* ans_open_text, int* ans_quantization) {
    //Delete message which is not reply
    if (!message->replyToMessage && message->text != "/start" && message->text != "/rerun") {
        bot.getApi().deleteMessage(message->chat->id, message->messageId);
    }

    if (StringTools::startsWith(query->data, "embed")) {
        cout << "embed" << endl;
        //Photo
        if (message->replyToMessage && ask_photo_embed == message->replyToMessage->text && (*ans_photo).empty() && (message->document || message->photo.size() > 0)) {
            prev_person_message = message;
            string save_img_res = embed_handler_photo(message, bot);
            if (!save_img_res.empty()) {
                *ans_photo = save_img_res;
                bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
                bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);
                prev_bot_message = bot.getApi().sendMessage(query->message->chat->id, ask_open_text, false, 0, cancel_btn);
                prev_person_message = nullptr;

            }
            else {
                bot.getApi().sendMessage(query->message->chat->id, "Image is incorrect, please try again!");
            }
        }
        //Message
        if (message->replyToMessage && ask_open_text == message->replyToMessage->text && (*ans_open_text).empty()) {
            prev_person_message = message;
            *ans_open_text = message->text;
            bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
            bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);
            prev_bot_message = bot.getApi().sendMessage(query->message->chat->id, ask_quantization_embed, false, 0, cancel_btn);
            prev_person_message = nullptr;
        }

        //Quantization
        if (message->replyToMessage && ask_quantization_embed == message->replyToMessage->text && !*ans_quantization) {
            prev_person_message = message;
            *ans_quantization = atoi(&(message->text)[0]);
            Message::Ptr message_prev = bot.getApi().sendMessage(query->message->chat->id, "Great! Now I am embedding your text in photo...", false, 0, cancel_btn);
            bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);
            bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
            int result = crypt_image(pathIn, *ans_photo, *ans_open_text, *ans_quantization);
            bot.getApi().deleteMessage(message->chat->id, message_prev->messageId);
            if (!result) {
                const string photoFilePath = pathIn + "output_" + *ans_photo;
                const string format = (*ans_photo).substr((*ans_photo).length() - 3, 3);
                const string photoMimeType = "image/png";
                bot.getApi().sendMessage(message->chat->id, "Your photo is ready, message: '" + *ans_open_text + "' has embedded.");
                bot.getApi().sendDocument(message->chat->id, InputFile::fromFile(photoFilePath, photoMimeType));
                string remove_photo = pathIn + "output_" + *ans_photo;
                remove(remove_photo.c_str());
            }
            else {
                bot.getApi().sendMessage(message->chat->id, "Something went wrong, try again!", false, 0, cancel_btn);
            }
            prev_bot_message = nullptr;
            prev_person_message = nullptr;
            prev_callback_ptr = nullptr;
            string remove_photo = pathIn + *ans_photo;
            remove(remove_photo.c_str());
            (*ans_photo).clear();
            (*ans_open_text).clear();
            *ans_quantization = 0;
            query->data.clear();
            bot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);
        }
    }

    if (StringTools::startsWith(query->data, "extract")) {
        cout << "extract" << endl;
        //Photo
        if (message->replyToMessage && ask_photo_extract == message->replyToMessage->text && (*ans_photo).empty() && message->document) {
            prev_person_message = message;
            string save_img_res = embed_handler_photo(message, bot);
            if (!save_img_res.empty()) {
                *ans_photo = save_img_res;
                bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
                bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);
                prev_bot_message = bot.getApi().sendMessage(query->message->chat->id, ask_quantization_extract, false, 0, cancel_btn);
                prev_person_message = nullptr;
            }
            else {
                bot.getApi().sendMessage(query->message->chat->id, "Image is incorrect, please try again!", false, 0, cancel_btn);
            }
        }

        //Quantization
        if (message->replyToMessage && ask_quantization_extract == message->replyToMessage->text && !*ans_quantization) {
            prev_person_message = message;
            *ans_quantization = atoi(&message->text[0]);
            string result = decrypt_image(pathIn, *ans_photo, *ans_quantization);
            if (result.empty()) {
                bot.getApi().sendMessage(query->message->chat->id, "There is not message in your photo");
            }
            else {
                bot.getApi().sendMessage(query->message->chat->id, "Your message is: " + result);
            }
            bot.getApi().deleteMessage(prev_person_message->chat->id, prev_person_message->messageId);
            bot.getApi().deleteMessage(prev_bot_message->chat->id, prev_bot_message->messageId);
            prev_person_message = nullptr;
            prev_bot_message = nullptr;
            prev_callback_ptr = nullptr;
            string remove_photo = pathIn + *ans_photo;
            remove(remove_photo.c_str());
            (*ans_photo).clear();
            (*ans_open_text).clear();
            *ans_quantization = 0;
            query->data.clear();
            bot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);
        }
    }
}

//Inline Keyboards
InlineKeyboardMarkup::Ptr create_cancel_button() {
    //Inline Cancel Markup Button
    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    vector<InlineKeyboardButton::Ptr> row0;
    InlineKeyboardButton::Ptr cancelButton(new InlineKeyboardButton);
    cancelButton->text = "Cancel!";
    cancelButton->callbackData = "cancel";
    row0.push_back(cancelButton);
    keyboard->inlineKeyboard.push_back(row0);
    
    return keyboard;
}
InlineKeyboardMarkup::Ptr create_keyboard() {
    //Inline Keyboard Markup
    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);

    vector<InlineKeyboardButton::Ptr> row0;
    vector<InlineKeyboardButton::Ptr> row1;

    InlineKeyboardButton::Ptr embedButton(new InlineKeyboardButton);
    InlineKeyboardButton::Ptr extractButton(new InlineKeyboardButton);

    embedButton->text = "Embed text in an image";
    embedButton->callbackData = "embed";
    row0.push_back(embedButton);

    extractButton->text = "Extract text from image";
    extractButton->callbackData = "extract";
    row1.push_back(extractButton);

    keyboard->inlineKeyboard.push_back(row0);
    keyboard->inlineKeyboard.push_back(row1);

    return keyboard;
}

// Photo handlers and helpers
size_t write_callback_helper(void* contents, size_t size, size_t nmemb, void* userp) {
    ofstream* file = static_cast<ofstream*>(userp);
    file->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}
int save_photo_from_tg(string token, string file_path, string file_name) {
    string imageUrl = "https://api.telegram.org/file/bot" + token + "/" + file_path;
    ofstream outFile(file_name, ofstream::binary);
    if (!outFile) {
        cerr << "Не удалось открыть файл для записи." << endl;
        return 1;
    }
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, imageUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_helper);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);

        CURLcode result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            cerr << "Ошибка при загрузке фотографии: " << curl_easy_strerror(result) << endl;
            return 1;
        }

        curl_easy_cleanup(curl);
    }
    else {
        cerr << "Ошибка при инициализации библиотеки libcurl." << endl;
        return 1;
    }
    cout << "Фотография успешно сохранена как " << file_name << endl;
    return 0;
}
string embed_handler_photo(Message::Ptr message, Bot& bot) {
    if (message->document) {
        Document::Ptr photo = message->document;
        string file_id = photo->fileId;
        string file_path = bot.getApi().getFile(file_id)->filePath;
        string file_name = to_string(message->chat->id) + "_" + to_string(message->messageId) + file_path.substr(file_path.length() - 4, 4);
        int result = save_photo_from_tg(bot.getToken(), file_path, file_name);
        if (result) {
            return "";
        }
        if (photo->mimeType == "image/jpeg") {
            return jpg_to_png(pathIn, file_name);
        }
        return file_name;
    }
    if (message->photo.size() > 0) {
        PhotoSize::Ptr photo = message->photo.back();
        string file_id = photo->fileId;
        string file_path = bot.getApi().getFile(file_id)->filePath;
        string file_name = to_string(message->chat->id) + "_" + to_string(message->messageId) + file_path.substr(file_path.length() - 4, 4);
        int result = save_photo_from_tg(bot.getToken(), file_path, file_name);
        return jpg_to_png(pathIn, file_name);;
    }
}
