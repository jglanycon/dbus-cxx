// SPDX-License-Identifier: LGPL-3.0-or-later OR BSD-3-Clause
/***************************************************************************
 *   Copyright (C) 2009 by Rick L. Vinyard, Jr.                            *
 *   rvinyard@cs.nmsu.edu                                                  *
 *                                                                         *
 *   This file is part of the dbus-cxx library.                            *
 ***************************************************************************/
#include "signature.h"

#include "dbus-cxx-private.h"
#include "types.h"
#include "error.h"

static const char* LOGGER_NAME = "DBus.Signature";

namespace DBus {

class Signature::priv_data
{
public:
    priv_data() :
        m_valid( false )
    {
    }

    std::string m_signature;

    std::shared_ptr<priv::SignatureNode> m_startingNode;
    bool m_valid;
};

Signature::Signature() :
    m_priv( std::make_shared<priv_data>() ) {
}

Signature::Signature( const std::string& s, size_type pos, size_type n ):
    m_priv( std::make_shared<priv_data>() ) {
    m_priv->m_signature = std::string( s, pos, n );
    initialize();
}

Signature::Signature( const char* s ) :
    m_priv( std::make_shared<priv_data>() ) {
    m_priv->m_signature = std::string( s );
    initialize();
}

Signature::Signature( const char* s, size_type n ):
    m_priv( std::make_shared<priv_data>() ) {
    m_priv->m_signature = std::string( s, n );
    initialize();
}

Signature::Signature( size_type n, char c ):
    m_priv( std::make_shared<priv_data>() ) {
    m_priv->m_signature = std::string( n, c );
    initialize();
}

Signature::~Signature() {
}

Signature::operator const std::string& () const {
    return m_priv->m_signature;
}

const std::string& Signature::str() const {
    return m_priv->m_signature;
}

Signature& Signature::operator =( const std::string& s ) {
    m_priv->m_signature = s;
    return *this;
}

Signature& Signature::operator =( const char* s ) {
    m_priv->m_signature = s;
    return *this;
}

Signature::iterator Signature::begin() {
    if( !m_priv->m_valid ) { return SignatureIterator(); }

    return SignatureIterator( m_priv->m_startingNode );
}

Signature::const_iterator Signature::begin() const {
    if( !m_priv->m_valid ) { return SignatureIterator(); }

    return SignatureIterator( m_priv->m_startingNode );
}

Signature::iterator Signature::end() {
    return SignatureIterator( nullptr );
}

Signature::const_iterator Signature::end() const {
    return SignatureIterator( nullptr );
}

bool Signature::is_valid() const {
    return m_priv->m_valid;
}

bool Signature::is_singleton() const {
    return m_priv->m_valid &&
        m_priv->m_startingNode != nullptr &&
        m_priv->m_startingNode->m_dataType != DataType::INVALID  &&
        m_priv->m_startingNode->m_next == nullptr;
}

Signature::SignatureNodePointer Signature::create_signature_tree(
    std::string::const_iterator& itr,
    std::string::const_iterator const& end_itr,
    SignatureNodePointer const& parent_node)
{
    SignatureNodePointer first;
    SignatureNodePointer current;

    while (itr != end_itr)
    {
        DataType data_type =
            char_to_dbus_type(*itr);

        current =
            create_signature_node(
                data_type,
                current);

        if (!first)
        {
            first =
                current;
        }

        switch (data_type)
        {
            case DataType::ARRAY:
            {
                current->m_sub =
                    create_signature_tree(
                        ++itr,
                        end_itr,
                        current);

                break;
            }
            case DataType::STRUCT:
            {
                break;
            }
            case DataType::STRUCT_BEGIN:
            {
                current->m_sub =
                    create_signature_tree(
                        ++itr,
                        end_itr,
                        current);

                break;
            }
            case DataType::STRUCT_END:
            {
                if (parent_node->m_dataType != DataType::STRUCT)
                    throw ErrorUnableToParse("STRUCT_END end without any STRUCT_BEGIN");

                return first;
            }
            case DataType::DICT_ENTRY:
            {
                break;
            }
            case DataType::DICT_ENTRY_BEGIN:
            {
                current->m_sub =
                    create_signature_tree(
                        ++itr,
                        end_itr,
                        current);

                break;
            }
            case DataType::DICT_ENTRY_END:
            {
                if (parent_node->m_dataType != DataType::DICT_ENTRY)
                    throw ErrorUnableToParse("DICT_ENTRY END end without any DICT_ENTRY BEGIN");

                return first;
            }

            case DataType::BYTE:
            case DataType::BOOLEAN:
            case DataType::INT16:
            case DataType::UINT16:
            case DataType::INT32:
            case DataType::UINT32:
            case DataType::INT64:
            case DataType::UINT64:
            case DataType::DOUBLE:
            case DataType::STRING:
            case DataType::OBJECT_PATH:
            case DataType::SIGNATURE:
            case DataType::VARIANT:
            case DataType::UNIX_FD:
            {
                break;
            }

            default:
            {
                throw ErrorUnableToParse("Unknown DataType");
            }
        }

        if ( (parent_node) &&
             (parent_node->m_dataType == DataType::ARRAY) )
        {
            return first;
        }

        ++itr;
    }

    if ( (parent_node) &&
         (parent_node->m_dataType == DataType::STRUCT) )
        throw ErrorUnableToParse("Missing STRUCT_END");

    if ( (parent_node) &&
         (parent_node->m_dataType == DataType::DICT_ENTRY) )
        throw ErrorUnableToParse("Missing DICT_ENTRY_END");

    return first;
}

Signature::SignatureNodePointer Signature::create_signature_node(
    DataType data_type,
    SignatureNodePointer const& current_node) const
{
    SignatureNodePointer new_node;

    if ( (data_type == DataType::STRUCT_END) ||
         (data_type == DataType::DICT_ENTRY_END) )
    {
        return current_node;
    }

    if (data_type == DataType::STRUCT_BEGIN)
    {
        // TODO: Must the STRUCT_BEGIN...
        new_node =
            std::make_shared<priv::SignatureNode>(
                DataType::STRUCT);
    }
    else if (data_type == DataType::DICT_ENTRY_BEGIN)
    {
        // TODO: Must the DICT_ENTRY_BEGIN...
        new_node =
            std::make_shared<priv::SignatureNode>(
                DataType::DICT_ENTRY);
    }
    else
    {
        new_node =
            std::make_shared<priv::SignatureNode>(
                data_type);
    }

    if (current_node)
    {
        current_node->m_next =
            new_node;
    }

    return new_node;
}

void Signature::print_tree( std::ostream* stream ) const {
    SignatureNodePointer current = m_priv->m_startingNode;

    while( current != nullptr ) {
        *stream << current->m_dataType;
        current = current->m_next;

        if( current == nullptr ) {
            *stream << " (null) ";
        } else {
            *stream << " --> ";
        }
    }
}

void Signature::print_node( std::ostream* stream, priv::SignatureNode* node, int spaces ) const
{
    if( node == nullptr ) {
        return;
    }

    for( int x = 0; x < spaces; x++ ) {
        *stream << " ";
    }

    *stream << node->m_dataType;
}

void Signature::initialize() {
    m_priv->m_valid = true;

    std::string::const_iterator itr =
        m_priv->m_signature.begin();

    try
    {
        m_priv->m_startingNode = create_signature_tree(
            itr,
            m_priv->m_signature.end());
    }
    catch (ErrorUnableToParse const& exception)
    {
        std::ostringstream error_message;
        error_message << "Unable to parse signature with error \'" << exception.what() << "\'";
        SIMPLELOGGER_DEBUG(LOGGER_NAME, error_message.str());
        m_priv->m_valid = false;
    }

    std::ostringstream logmsg;
    logmsg << "Signature \'" << m_priv->m_signature << "\' is " << (m_priv->m_valid ? "valid" : "invalid") << "\'";

    SIMPLELOGGER_TRACE( LOGGER_NAME, logmsg.str() );
}

}
