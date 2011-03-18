//
//  Copyright Mathieu Champlon 2011
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MOCK_LOG_HPP_INCLUDED
#define MOCK_LOG_HPP_INCLUDED

#include "is_container.hpp"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/function_types/is_callable_builtin.hpp>
#include <iostream>
#include <string>

namespace mock
{
    struct stream
    {
        explicit stream( std::ostream& s )
            : s_( &s )
        {}
        std::ostream* s_;
    };

namespace detail3
{
    template< typename T >
    struct serializer
    {
        explicit serializer( const T& t )
            : t_( &t )
        {}
        void serialize( stream& s ) const
        {
            // if it fails here because ambiguous stuff
            // -> add operator<< to mock::stream for T
            s << *t_;
        }
        const T* t_;
    };
    template<>
    struct serializer< bool >
    {
        explicit serializer( bool b )
            : b_( b )
        {}
        void serialize( stream& s ) const
        {
            *s.s_ << std::boolalpha << b_;
        }
        bool b_;
    };
    template<>
    struct serializer< const char* >
    {
        explicit serializer( const char* s )
            : s_( s )
        {}
        void serialize( stream& s ) const
        {
            *s.s_ << '"' << s_ << '"';
        }
        const char* s_;
    };
    template<>
    struct serializer< std::string >
    {
        explicit serializer( const std::string& s )
            : s_( &s )
        {}
        void serialize( stream& s ) const
        {
            *s.s_ << '"' << *s_ << '"';
        }
        const std::string* s_;
    };

    template< typename T >
    stream& operator<<( stream& s, const serializer< T >& ser )
    {
        ser.serialize( s );
        return s;
    }

    template< typename T >
    std::ostream& operator<<( std::ostream& s, const serializer< T >& ser )
    {
        stream ss( s );
        ser.serialize( ss );
        return s;
    }
}
    template< typename T >
    detail3::serializer< T > format( const T& t )
    {
        return detail3::serializer< T >( t );
    }
    inline detail3::serializer< const char* > format( const char* s )
    {
        return detail3::serializer< const char* >( s );
    }

#ifdef MOCK_LOG_CONVERSIONS

namespace detail
{
    struct sink
    {
        template< typename T >
        sink( const T&, BOOST_DEDUCED_TYPENAME boost::disable_if<
                boost::is_fundamental< T > >::type* = 0 )
        {}
    };

    std::ostream& operator<<( std::ostream& s, const detail::sink& )
    {
        return s << "?";
    }

    struct holder
    {
        virtual ~holder()
        {}
        virtual void serialize( std::ostream& s ) const = 0;
    };

    template< typename T >
    struct holder_imp : holder
    {
        explicit holder_imp( const T& t )
            : t_( &t )
        {}
        virtual void serialize( std::ostream& s ) const
        {
            // if it fails here because ambiguous stuff
            // -> add operator<< to mock::stream for T
            s << *t_;
        }
        const T* t_;
    };

    struct any
    {
        template< typename T >
        any( const T& t )
            : h_( new holder_imp< T >( t ) )
        {}
        ~any()
        {
            delete h_;
        }
        holder* h_;
    };
}

    stream& operator<<( stream& s, const detail::any& d )
    {
        d.h_->serialize( *s.s_ );
        return s;
    }

#else // MOCK_LOG_CONVERSIONS

namespace detail
{
    template< typename S, typename T >
    S& operator<<( S &s, const T& )
    {
        return s << "?";
    }
}

    template< typename T >
    void serialize( std::ostream& s, const T& t )
    {
        using namespace detail;
        s << t;
    }

namespace detail2
{
    template< typename S, typename T >
    S& operator<<( S& s, const T& t )
    {
        serialize( s, t );
        return s;
    }
}

    template< typename T >
    BOOST_DEDUCED_TYPENAME boost::disable_if<
            detail::is_container< T >, stream&
    >::type
    operator<<( stream& s, const T& t )
    {
        using namespace detail2;
        *s.s_ << t;
        return s;
    }

#endif // MOCK_LOG_CONVERSIONS

    template< typename T1, typename T2 >
    stream& operator<<( stream& s, const std::pair< T1, T2 >& p )
    {
        return s << '(' << mock::format( p.first )
            << ',' << mock::format( p.second ) << ')';
    }

    template< typename Container >
    BOOST_DEDUCED_TYPENAME boost::enable_if<
            detail::is_container< Container >, stream&
    >::type
    operator<<( stream& s, const Container& c )
    {
        s << '(';
        // if an error is generated by the line below it means Container is
        // being mismatched for a container because it has a typedef
        // const_iterator : the easiest solution would be to add a format
        // function for Container as well.
        for( BOOST_DEDUCED_TYPENAME Container::const_iterator it = c.begin();
            it != c.end(); ++it )
        {
            if( it != c.begin() )
                s << ',';
            s << mock::format( *it );
        }
        return s << ')';
    }

    //template< typename T >
    //BOOST_DEDUCED_TYPENAME boost::enable_if<
    //        boost::function_types::is_callable_builtin< T >, stream&
    //>::type
    //operator<<( stream& s, const T& )
    //{
    //     return s << '?';
    //}
}

#endif // #ifndef MOCK_LOG_HPP_INCLUDED
