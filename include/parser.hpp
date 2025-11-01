#ifndef PARSER_HPP
#define PARSER_HPP

#include "thread_pool.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>


enum class NodeType { Element, Text, Comment };

struct Node {
    NodeType type;
    std::string name;
    std::string text;
    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;
    std::unordered_map<std::string,std::string> attributes;
};


class Parser {


    public:

        static Parser& instance(ThreadPool& pool){
            static Parser parser(pool);
            return parser;
        }

        Parser& operator=(const Parser&) = delete;
        Parser(const Parser&) = delete;




    private:

        Parser(ThreadPool& pool): pool_(pool) {

        }

        std::vector<Node> stack_;
        ThreadPool& pool_;



};

#endif