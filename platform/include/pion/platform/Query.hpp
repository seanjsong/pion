// ------------------------------------------------------------------------
// Pion is a development platform for building Reactors that process Events
// ------------------------------------------------------------------------
// Copyright (C) 2007-2008 Atomic Labs, Inc.  (http://www.atomiclabs.com)
//
// Pion is free software: you can redistribute it and/or modify it under the
// terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// Pion is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
// more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with Pion.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __PION_QUERY_HEADER__
#define __PION_QUERY_HEADER__

#include <string>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <pion/PionConfig.hpp>
#include <pion/platform/Vocabulary.hpp>
#include <pion/platform/Event.hpp>


namespace pion {		// begin namespace pion
namespace platform {	// begin namespace platform (Pion Platform Library)

// forward declarations used by Query
class Database;


///
/// Query: abstract class for representing a compiled database query
///
class Query
	: private boost::noncopyable
{
public:

	/// data type for a pair where first is a field name, and second is Term info
	typedef std::pair<std::string, Vocabulary::Term>		FieldData;

	/// data type for a map of Term references to database field names
	typedef std::map<Vocabulary::TermRef, FieldData>		FieldMap;


	/// virtual destructor -> this class is just an interface
	virtual ~Query() {}

	/**
	 * binds a NULL value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 */
	virtual void bindNull(unsigned int param) = 0;

	/**
	 * binds a std::string value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 * @param copy_value if true, the string will be copied into a temporary buffer
	 */
	virtual void bindString(unsigned int param, const std::string& value, bool copy_value = true) = 0;

	virtual void fetchString(unsigned int param, std::string& value) = 0;

	/**
	 * binds a string (const char *) value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 * @param copy_value if true, the string will be copied into a temporary buffer
	 */
	virtual void bindString(unsigned int param, const char *value, bool copy_value = true) = 0;

//	virtual char *fetchString(unsigned int param) = 0;

	/**
	 * binds an integer value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindInt(unsigned int param, const boost::int32_t value) = 0;

	virtual boost::int32_t fetchInt(unsigned int param) = 0;

	/**
	 * binds an unsigned integer value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindUInt(unsigned int param, const boost::uint32_t value) = 0;

	virtual boost::uint32_t fetchUInt(unsigned int param) = 0;

	/**
	 * binds a big integer value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindBigInt(unsigned int param, const boost::int64_t value) = 0;

	virtual boost::int64_t fetchBigInt(unsigned int param) = 0;

	/**
	 * binds an unsigned big integer value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindUBigInt(unsigned int param, const boost::uint64_t value) = 0;

	virtual boost::uint64_t fetchUBigInt(unsigned int param) = 0;

	/**
	 * binds a floating point number value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindFloat(unsigned int param, const float value) = 0;

	virtual float fetchFloat(unsigned int param) = 0;

	/**
	 * binds a double floating point number value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindDouble(unsigned int param, const double value) = 0;

	virtual double fetchDouble(unsigned int param) = 0;

	/**
	 * binds a long double floating point number value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindLongDouble(unsigned int param, const long double value) = 0;

	virtual long double fetchLongDouble(unsigned int param) = 0;

	/**
	 * binds a date_time value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindDateTime(unsigned int param, const PionDateTime& value) = 0;

	virtual void fetchDateTime(unsigned int param, PionDateTime& val) = 0;

	/**
	 * binds a date value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindDate(unsigned int param, const PionDateTime& value) = 0;

	virtual void fetchDate(unsigned int param, PionDateTime& val) = 0;

	/**
	 * binds a time value to a query parameter
	 *
	 * @param param the query parameter number to which the value will be bound (starting with 0)
	 * @param value the value to bind to the query parameter
	 */
	virtual void bindTime(unsigned int param, const PionDateTime& value) = 0;

	virtual void fetchTime(unsigned int param, PionDateTime& val) = 0;

	/**
	 * binds the data contained within an Event to the query parameters
	 * (this asssumes that field_map is ordered the same as the parameters)
	 *
	 * @param field_map mapping of Vocabulary Terms to Database fields
	 * @param e the Event containing data to bind to the query
	 * @param copy_strings if true, the strings will be copied into temporary buffers
	 */
	inline void bindEvent(const FieldMap& field_map, const Event& e, bool copy_strings = true);

	inline void fetchEvent(const FieldMap& field_map, EventPtr e);

	/**
	 * runs the compiled query
	 *
	 * @return true if there is a result row available
	 */
	virtual bool run(void) = 0;

	/// resets the compiled query so that it can be run again
	virtual void reset(void) = 0;

	/// returns the SQL that was compiled to initialize this query
	inline const std::string& getSQL(void) const { return m_sql_query; }


protected:

	/**
	 * this class can only be constructed by its descendants
	 *
	 * @param sql_query the SQL to use to initialize this query
	 */
	Query(const std::string& sql_query)
		: m_sql_query(sql_query)
	{}


private:

	/// the SQL that was compiled to initialize this query
	const std::string					m_sql_query;
};


// inline member functions for Query

inline void Query::bindEvent(const FieldMap& field_map, const Event& e, bool copy_strings)
{
	unsigned int param = 0;
	for (Query::FieldMap::const_iterator field_it = field_map.begin();
		 field_it != field_map.end(); ++field_it)
	{
		const Event::ParameterValue *value_ptr = e.getPointer(field_it->first);
		if (value_ptr == NULL) {
			bindNull(param);
		} else {
			switch(field_it->second.second.term_type) {
				case Vocabulary::TYPE_NULL:
				case Vocabulary::TYPE_OBJECT:
					bindNull(param);
					break;
				case Vocabulary::TYPE_INT8:
				case Vocabulary::TYPE_INT16:
				case Vocabulary::TYPE_INT32:
					bindInt(param, boost::get<boost::int32_t>(*value_ptr));
					break;
				case Vocabulary::TYPE_INT64:
					bindBigInt(param, boost::get<boost::int64_t>(*value_ptr));
					break;
				case Vocabulary::TYPE_UINT8:
				case Vocabulary::TYPE_UINT16:
				case Vocabulary::TYPE_UINT32:
					bindUInt(param, boost::get<boost::uint32_t>(*value_ptr));
					break;
				case Vocabulary::TYPE_UINT64:
					bindUBigInt(param, boost::get<boost::uint64_t>(*value_ptr));
					break;
				case Vocabulary::TYPE_FLOAT:
					bindFloat(param, boost::get<float>(*value_ptr));
					break;
				case Vocabulary::TYPE_DOUBLE:
					bindDouble(param, boost::get<double>(*value_ptr));
					break;
				case Vocabulary::TYPE_LONG_DOUBLE:
					bindLongDouble(param, boost::get<long double>(*value_ptr));
					break;
				case Vocabulary::TYPE_SHORT_STRING:
				case Vocabulary::TYPE_STRING:
				case Vocabulary::TYPE_LONG_STRING:
				case Vocabulary::TYPE_CHAR:
				case Vocabulary::TYPE_REGEX:
					bindString(param,
						boost::get<const Event::SimpleString&>(*value_ptr).get(),
						copy_strings);
					break;
				case Vocabulary::TYPE_DATE_TIME:
					bindDateTime(param, boost::get<const PionDateTime&>(*value_ptr));
					break;
				case Vocabulary::TYPE_DATE:
					bindDate(param, boost::get<const PionDateTime&>(*value_ptr));
					break;
				case Vocabulary::TYPE_TIME:
					bindTime(param, boost::get<const PionDateTime&>(*value_ptr));
					break;
			}
		}
		++param;
	}
}

inline void Query::fetchEvent(const FieldMap& field_map, EventPtr e)
{
	unsigned int param = 0;
	for (Query::FieldMap::const_iterator field_it = field_map.begin();
		 field_it != field_map.end(); ++field_it)
	{
		switch(field_it->second.second.term_type) {
			case Vocabulary::TYPE_NULL:
			case Vocabulary::TYPE_OBJECT:
				break;
			case Vocabulary::TYPE_INT8:
			case Vocabulary::TYPE_INT16:
			case Vocabulary::TYPE_INT32:
				e->setInt(field_it->first, fetchInt(param));
				break;
			case Vocabulary::TYPE_INT64:
				e->setBigInt(field_it->first, fetchBigInt(param));
				break;
			case Vocabulary::TYPE_UINT8:
			case Vocabulary::TYPE_UINT16:
			case Vocabulary::TYPE_UINT32:
				e->setUInt(field_it->first, fetchUInt(param));
				break;
			case Vocabulary::TYPE_UINT64:
				e->setUBigInt(field_it->first, fetchUBigInt(param));
				break;
			case Vocabulary::TYPE_FLOAT:
				e->setFloat(field_it->first, fetchFloat(param));
				break;
			case Vocabulary::TYPE_DOUBLE:
				e->setDouble(field_it->first, fetchDouble(param));
				break;
			case Vocabulary::TYPE_LONG_DOUBLE:
				e->setLongDouble(field_it->first, fetchLongDouble(param));
				break;
			case Vocabulary::TYPE_SHORT_STRING:
			case Vocabulary::TYPE_STRING:
			case Vocabulary::TYPE_LONG_STRING:
			case Vocabulary::TYPE_CHAR:
			case Vocabulary::TYPE_REGEX:
				{
					std::string val;
					fetchString(param, val);
					e->setString(field_it->first, val);
				}
				break;
			case Vocabulary::TYPE_DATE_TIME:
				{
					PionDateTime val;
					fetchDateTime(param, val);
					e->setDateTime(field_map[param].second.term_ref, val);
				}
				break;
			case Vocabulary::TYPE_DATE:
				{
					PionDateTime val;
					fetchDate(param, val);
					e->setDateTime(field_map[param].second.term_ref, val);
				}
				break;
			case Vocabulary::TYPE_TIME:
				{
					PionDateTime val;
					fetchTime(param, val);
					e->setDateTime(field_it->first, val);
				}
				break;
		}
		++param;
	}
}


/// data type used to uniquely identify a specific type of database query
typedef std::string						QueryID;


/// data type used for Query smart pointers
typedef boost::shared_ptr<Query>		QueryPtr;


}	// end namespace platform
}	// end namespace pion

#endif
