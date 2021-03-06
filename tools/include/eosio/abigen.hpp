#pragma once
#include <eosio/gen.hpp>

#include <eosio/utils.hpp>
#include <eosio/whereami/whereami.hpp>
#include <eosio/abi.hpp>

#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <set>
#include <map>

#include <jsoncons/json.hpp>

using namespace llvm;
using namespace eosio;
using namespace eosio::cdt;
using jsoncons::json;
using jsoncons::ojson;

namespace eosio { namespace cdt {
   struct abigen_exception : public std::exception {
      virtual const char* what() const throw() {
         return "eosio.abigen fatal error";
      }
   };
   extern abigen_exception abigen_ex;

   class abigen : public generation_utils {
      public:

      abigen() : generation_utils([&](){throw abigen_ex;}) {
      }

      void add_typedef( const clang::QualType& t ) {
         abi_typedef ret;
         ret.new_type_name = get_base_type_name( t );
         auto td = get_type_alias(t);
         if (td.empty())
            return;
         ret.type = translate_type(td[0]);
         if(!is_builtin_type(td[0]))
            add_type(td[0]);
         _abi.typedefs.insert(ret);
      }

      void add_action( const clang::CXXRecordDecl* decl ) {
         abi_action ret;
         auto action_name = decl->getEosioActionAttr()->getName();

         if (action_name.empty()) {
            try {
               validate_name( decl->getName().str(), error_handler );
            } catch (...) {
               std::cout << "Error, name <" <<decl->getName().str() << "> is an invalid EOSIO name.\n";
               throw;
            }
            ret.name = decl->getName().str();
         }
         else {
            try {
               validate_name( action_name.str(), error_handler );
            } catch (...) {
               std::cout << "Error, name <" << action_name.str() << "> is an invalid EOSIO name.\n";
               throw;
            }
            ret.name = action_name.str();
         }
         ret.type = decl->getName().str();
         _abi.actions.insert(ret);
      }

      void add_action( const clang::CXXMethodDecl* decl ) {
         abi_action ret;

         auto action_name = decl->getEosioActionAttr()->getName();

         if (action_name.empty()) {
            try {
               validate_name( decl->getNameAsString(), error_handler );
            } catch (...) {
               std::cout << "Error, name <" <<decl->getNameAsString() << "> is an invalid EOSIO name.\n";
            }
            ret.name = decl->getNameAsString();
         }
         else {
            try {
               validate_name( action_name.str(), error_handler );
            } catch (...) {
               std::cout << "Error, name <" << action_name.str() << "> is an invalid EOSIO name.\n";
            }
            ret.name = action_name.str();
         }
         ret.type = decl->getNameAsString();
         _abi.actions.insert(ret);
      }

      void add_event( const clang::CXXRecordDecl* decl ) {
         abi_event t;
         t.type = decl->getNameAsString();
         auto event_name = decl->getEosioEventAttr()->getName();
         if (event_name.empty()) {
            auto struct_name = t.type;
            auto postfix = struct_name.rfind("_event");
            if (postfix != std::string::npos) struct_name.resize(postfix);
            try {
               validate_name( struct_name, error_handler );
            } catch (...) {
               std::cout << "Error, name <" << struct_name << "> is an invalid EOSIO name.\n";
            }
            t.name = struct_name;
         } else {
            try {
               validate_name( event_name.str(), error_handler );
            } catch (...) {
               std::cout << "Error, name <" << event_name.str() << "> is an invalid EOSIO name.\n";
            }
            t.name = event_name.str();
         }
         _abi.events.insert(t);
      }

      void add_tuple(const clang::QualType& type) {
         auto pt = llvm::dyn_cast<clang::ElaboratedType>(type.getTypePtr());
         auto tst = llvm::dyn_cast<clang::TemplateSpecializationType>((pt) ? pt->desugar().getTypePtr() : type.getTypePtr());
         if (!tst)
            throw abigen_ex;
         abi_struct tup;
         tup.name = get_type(type);
         for (int i = 0; i < tst->getNumArgs(); ++i) {
            clang::QualType ftype = get_template_argument(type, i).getAsType();
            add_type(ftype);
            tup.fields.push_back( {"field_"+std::to_string(i),
                  translate_type(ftype)} );
         }
         _abi.structs.insert(tup);
      }

      void add_pair(const clang::QualType& type) {
         for (int i = 0; i < 2; ++i) {
            clang::QualType ftype = get_template_argument(type, i).getAsType();
            std::string ty = translate_type(ftype);
            add_type(ftype);
         }
         abi_struct pair;
         pair.name = get_type(type);
         pair.fields.push_back( {"first", translate_type(get_template_argument(type).getAsType())} );
         pair.fields.push_back( {"second", translate_type(get_template_argument(type, 1).getAsType())} );   
         add_type(get_template_argument(type).getAsType());
         add_type(get_template_argument(type, 1).getAsType());
         _abi.structs.insert(pair);
      } 

      void add_map(const clang::QualType& type) {
         for (int i = 0; i < 2; ++i) {
            clang::QualType ftype = get_template_argument(type, i).getAsType();
            std::string ty = translate_type(ftype);
            add_type(ftype);
         }
         abi_struct kv;
         std::string name = get_type(type);
         kv.name = name.substr(0, name.length() - 2);
         kv.fields.push_back( {"key", translate_type(get_template_argument(type).getAsType())} );
         kv.fields.push_back( {"value", translate_type(get_template_argument(type, 1).getAsType())} );   
         add_type(get_template_argument(type).getAsType());
         add_type(get_template_argument(type, 1).getAsType());
         _abi.structs.insert(kv);
      }

      void add_struct( const clang::CXXRecordDecl* decl, const std::string& rname="", bool add_id = false ) {
         abi_struct ret;
         if ( decl->getNumBases() == 1 ) {
            ret.base = get_type(decl->bases_begin()->getType());
            add_type(decl->bases_begin()->getType());
         }
         if (add_id) {
            ret.fields.push_back({"id", "uint64"});
         }
         for ( auto field : decl->fields() ) {
            if ( field->getName() == "transaction_extensions") {
               abi_struct ext;
               ext.name = "extension";
               ext.fields.push_back( {"type", "uint16"} );
               ext.fields.push_back( {"data", "bytes"} );
               ret.fields.push_back( {"transaction_extensions", "extension[]"});
               _abi.structs.insert(ext);
            }
            else {
               ret.fields.push_back({field->getName().str(), get_type(field->getType())});
               add_type(field->getType());
            }
         }
         if (!rname.empty())
            ret.name = rname;
         else
            ret.name = decl->getName().str();
         _abi.structs.insert(ret);
      }

      void add_struct( const clang::CXXMethodDecl* decl ) {
         abi_struct new_struct;
         new_struct.name = decl->getNameAsString();
         for (auto param : decl->parameters() ) {
            auto param_type = param->getType().getNonReferenceType().getUnqualifiedType();
            new_struct.fields.push_back({param->getNameAsString(), get_type(param_type)});
            add_type(param_type);
         }
         _abi.structs.insert(new_struct);
      }

      std::string get_template_arg_as_name(const clang::Decl* decl, size_t arg_idx = 0) {
         const auto* im = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl);
         auto name_raw = im->getTemplateArgs()[arg_idx].getAsIntegral().getExtValue();
         return name_to_string(name_raw);
      }

      void add_index(const clang::TypedefNameDecl* decl, const clang::TemplateSpecializationType* templ, std::string idx_name = "") {
         abi_index idx;
         if (!idx_name.empty()) {
            idx.name = idx_name;
         } else {
            const auto* rec = clang::dyn_cast<clang::RecordType>(templ->desugar())->getDecl();
            idx.name = get_template_arg_as_name(rec);
         }

         idx.unique = !decl->hasEosioNonUnique();

         for (auto* attr: decl->getAttrs()) {
            if (auto* order = llvm::dyn_cast<clang::EosioOrderAttr>(attr)) {
               // Clang provides attributes in reversed order, so insert to front
               idx.orders.insert(idx.orders.begin(), abi_order{order->getField(), order->getOrder()});
            }
         }

         indexes[idx.name] = idx;
      }

      void add_table(const clang::TypedefNameDecl* decl, const clang::TemplateSpecializationType* templ, bool is_singleton = false) {
         if (!abigen::is_eosio_contract(decl, get_contract_name()))
            return;

         abi_table t;
         t.name = get_template_arg_as_name(clang::dyn_cast<clang::RecordType>(templ->desugar())->getDecl());

         const auto* structure = templ->getArg(1).getAsType().getTypePtr()->getAsCXXRecordDecl();
         add_struct(structure, "", is_singleton);
         t.type = structure->getNameAsString();

         if (decl->hasEosioScopeType()) {
            t.scope_type = decl->getEosioScopeType();
         }

         if (!is_singleton) {
            add_index(decl, templ, "primary");
            t.indexes.push_back(indexes["primary"]);

            for (int i = 2; i < templ->getNumArgs(); ++i) {
               const auto& idx_name = get_template_arg_as_name(templ->getArg(i).getAsType().getTypePtr()->getAsCXXRecordDecl());
               t.indexes.push_back(indexes[idx_name]);
            }
         } else {
            abi_index idx;
            idx.name = "primary";
            idx.unique = true;
            idx.orders.push_back(abi_order{"id", "asc"});
            t.indexes.push_back(idx);
         }

         ctables.insert(t);
      }

      void add_table( const clang::CXXRecordDecl* decl ) {
         tables.insert(decl);
         abi_table t;
         t.type = decl->getNameAsString();
         auto table_name = decl->getEosioTableAttr()->getName();
         if (!table_name.empty()) {
            try {
               validate_name( table_name.str(), error_handler );
            } catch (...) {
            }
            t.name = table_name.str();
         }
         else {
            t.name = t.type;
         }
         ctables.insert(t);
      }

      void add_table( uint64_t name, const clang::CXXRecordDecl* decl ) {
         if (!(decl->isEosioTable() && abigen::is_eosio_contract(decl, get_contract_name())))
            return;
         abi_table t;
         t.type = decl->getNameAsString();
         t.name = name_to_string(name);
         _abi.tables.insert(t);
      }

      void add_variant( const clang::QualType& t ) {
         abi_variant var;
         auto pt = llvm::dyn_cast<clang::ElaboratedType>(t.getTypePtr());
         auto tst = llvm::dyn_cast<clang::TemplateSpecializationType>(pt->desugar().getTypePtr());
         var.name = get_type(t);
         for (int i=0; i < tst->getNumArgs(); ++i) {
            var.types.push_back(translate_type(get_template_argument( t, i ).getAsType()));
            add_type(get_template_argument( t, i ).getAsType());
         }
         _abi.variants.insert(var); 
      }

      void add_type( const clang::QualType& t ) {
         if (evaluated.count(t.getTypePtr()))
            return;
         evaluated.insert(t.getTypePtr());
         auto type = get_ignored_type(t);
         if (!is_builtin_type(translate_type(type, false))) {
            if (is_aliasing(type))
               add_typedef(type);
            else if (is_template_specialization(type, {"vector", "set", "deque", "list", "optional", "binary_extension", "ignore"})) {
               add_type(get_template_argument(type).getAsType());
            }
            else if (is_template_specialization(type, {"map"}))
               add_map(type);
            else if (is_template_specialization(type, {"pair"}))
               add_pair(type);
            else if (is_template_specialization(type, {"tuple"}))
               add_tuple(type);
            else if (is_template_specialization(type, {"variant"}))
               add_variant(type);
            else if (is_template_specialization(type, {})) {
               add_struct(type.getTypePtr()->getAsCXXRecordDecl(), get_template_name(type));
            }
            else if (type.getTypePtr()->isRecordType())
               add_struct(type.getTypePtr()->getAsCXXRecordDecl());
         }
      }

      std::string generate_json_comment() {
         std::stringstream ss;
         ss << "This file was generated with eosio-abigen.";
         ss << " DO NOT EDIT ";
         return ss.str(); 
      }

      ojson struct_to_json( const abi_struct& s ) {
         ojson o;
         o["name"] = s.name;
         o["base"] = s.base;
         o["fields"] = ojson::array();
         for ( auto field : s.fields ) {
            ojson f;
            f["name"] = field.name;
            f["type"] = field.type;
            o["fields"].push_back(f);
         }
         return o;
      }

      ojson variant_to_json( const abi_variant& v ) {
         ojson o;
         o["name"] = v.name;
         o["types"] = ojson::array();
         for ( auto ty : v.types ) {
            o["types"].push_back( ty );
         }
         return o;
      }

      ojson typedef_to_json( const abi_typedef& t ) {
         ojson o;
         o["new_type_name"] = t.new_type_name;
         o["type"]          = t.type;
         return o;
      }

      ojson action_to_json( const abi_action& a ) {
         ojson o;
         o["name"] = a.name;
         o["type"] = a.type;
         return o;
      }

      ojson event_to_json( const abi_event& e ) {
         ojson o;
         o["name"] = e.name;
         o["type"] = e.type;
         return o;
      }

      ojson table_to_json( const abi_table& t ) {
         ojson o;
         o["name"] = t.name;
         o["type"] = t.type;
         if (t.scope_type.size())
            o["scope_type"] = t.scope_type;
         o["indexes"] = ojson::array();
         for ( auto& index : t.indexes ) {
            ojson i;
            i["name"] = index.name;
            i["unique"] = index.unique;
            i["orders"] = ojson::array();
            for ( auto& order : index.orders ) {
               ojson ord;
               ord["field"] = order.field;
               ord["order"] = order.order;
               i["orders"].push_back(ord);
            }
            o["indexes"].push_back(i);
         }
          return o;
       }
      
      bool is_empty() {
         std::set<abi_table> set_of_tables;
         for ( auto t : ctables ) {
            bool has_multi_index = false;
            for ( auto u : _abi.tables ) {
               if (t.type == u.type) {
                  has_multi_index = true;
                  break;
               }
               set_of_tables.insert(u);
            }
            if (!has_multi_index)
               set_of_tables.insert(t);
         }
         for ( auto t : _abi.tables ) {
            set_of_tables.insert(t);
         }

         return _abi.structs.empty() && _abi.typedefs.empty() && _abi.actions.empty() && _abi.events.empty() && set_of_tables.empty() && _abi.variants.empty();
      }

      ojson to_json() {
         ojson o;
         o["____comment"] = generate_json_comment();
         o["version"]     = _abi.version;
         o["structs"]     = ojson::array();
         auto remove_suffix = [&]( std::string name ) {
            int i = name.length()-1;
            for (; i >= 0; i--) 
               if ( name[i] != '[' && name[i] != ']' && name[i] != '?' && name[i] != '$' )
                  break;
            return name.substr(0,i+1);
         };

         std::set<abi_table> set_of_tables;
         for ( auto t : ctables ) {
            bool has_multi_index = false;
            for ( auto u : _abi.tables ) {
               if (t.type == u.type) {
                  has_multi_index = true;
                  break;
               }
               set_of_tables.insert(u);
            }
            if (!has_multi_index)
               set_of_tables.insert(t);
         }
         for ( auto t : _abi.tables ) {
            set_of_tables.insert(t);
         }

         std::function<std::string(const std::string&)> get_root_name;
         get_root_name = [&] (const std::string& name) {
            for (auto td : _abi.typedefs)
               if (remove_suffix(name) == td.new_type_name)
                  return get_root_name(td.type);
            return name;
         };

         auto validate_struct = [&]( abi_struct as ) {
            if ( is_builtin_type(_translate_type(as.name)) )
               return false;
            for ( auto s : _abi.structs ) {
               for ( auto f : s.fields ) {
                  if (as.name == _translate_type(remove_suffix(f.type)))
                     return true;
               }
               for ( auto v : _abi.variants ) {
                  for ( auto vt : v.types ) {
                     if (as.name == _translate_type(remove_suffix(vt)))
                        return true;
                  }
               }
               if (get_root_name(s.base) == as.name)
                  return true;
            }
            for ( auto a : _abi.actions ) {
               if (as.name == _translate_type(a.type))
                  return true;
            }
            for ( auto a : _abi.events ) {
               if (as.name == _translate_type(a.type))
                  return true;
            }
            for( auto t : set_of_tables ) {
               if (as.name == _translate_type(t.type))
                  return true;
            }
            for( auto td : _abi.typedefs ) {
               if (as.name == _translate_type(remove_suffix(td.type)))
                  return true;
            }
            return false;
         };

         auto validate_types = [&]( abi_typedef td ) {
            for ( auto as : _abi.structs )
               if (validate_struct(as)) {
                  for ( auto f : as.fields )
                     if ( remove_suffix(f.type) == td.new_type_name )
                        return true;
                  if (as.base == td.new_type_name)
                     return true;
               }
            for ( auto v : _abi.variants ) {
               for ( auto vt : v.types ) {
                  if ( remove_suffix(vt) == td.new_type_name )
                     return true;
               }
            }
            for ( auto t : _abi.tables )
               if ( t.type == td.new_type_name )
                  return true;
            for ( auto a : _abi.actions )
               if ( a.type == td.new_type_name )
                  return true;
            for ( auto a : _abi.events )
               if ( a.type == td.new_type_name )
                  return true;
            for ( auto _td : _abi.typedefs )
               if ( remove_suffix(_td.type) == td.new_type_name )
                  return true;
            return false;
         };

         for ( auto s : _abi.structs ) {
            if (validate_struct(s))
               o["structs"].push_back(struct_to_json(s));
         }
         o["types"]       = ojson::array();
         for ( auto t : _abi.typedefs ) {
            if (validate_types(t))
               o["types"].push_back(typedef_to_json( t ));
         }
         o["actions"]     = ojson::array();
         for ( auto a : _abi.actions ) {
            o["actions"].push_back(action_to_json( a ));
         }
         o["events"]     = ojson::array();
         for ( auto a : _abi.events ) {
            o["events"].push_back(event_to_json( a ));
         }
         o["tables"]     = ojson::array();
         for ( auto t : set_of_tables ) {
            o["tables"].push_back(table_to_json( t ));
         }
         o["variants"]   = ojson::array();
         for ( auto v : _abi.variants ) {
            o["variants"].push_back(variant_to_json( v ));
         }
         o["abi_extensions"]     = ojson::array();
         return o;
      }

      private: 
         abi                                   _abi;
         std::map<std::string, abi_index>    indexes;
         std::set<const clang::CXXRecordDecl*> tables;
         std::set<abi_table>                   ctables;
         std::map<std::string, std::string>    rcs;
         std::set<const clang::Type*>          evaluated;
   };
}} // ns eosio::cdt
