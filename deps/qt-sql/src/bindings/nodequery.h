#ifndef NODEQUERY_H
#define NODEQUERY_H

#include <node.h>
#include <node_object_wrap.h>


class QSqlQuery;

namespace bindings {

class NodeQuery final : public node::ObjectWrap
{
public:
    NodeQuery();
    ~NodeQuery() override;

    static void init(v8::Local<v8::Object> exports);

    QSqlQuery *query() const;

private:
    static void newObject(const v8::FunctionCallbackInfo<v8::Value> &args);

    // Methods
    static void prepare(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void execute(const v8::FunctionCallbackInfo<v8::Value> &args);

    static void next(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void previous(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void first(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void last(const v8::FunctionCallbackInfo<v8::Value> &args);

    static void recordCount(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void fieldsCount(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void numRowsAffected(const v8::FunctionCallbackInfo<v8::Value> &args);
	  static void lastInsertId(const v8::FunctionCallbackInfo<v8::Value> &args);

    static void addParameter(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void fieldValue(const v8::FunctionCallbackInfo<v8::Value> &args);


    static void lastError(const v8::FunctionCallbackInfo<v8::Value> &args);

private:
    std::unique_ptr<QSqlQuery> innerQuery;
};

} // namespace bindings

#endif // NODEQUERY_H
