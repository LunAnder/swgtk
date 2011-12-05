
#include "datatable_reader.h"

#include <cstring>
#include <algorithm>
#include <iterator>
#include <boost/lexical_cast.hpp>

#include "utilities.h"

using namespace std;
using namespace swgtk;

struct IffHeader
{
    char form[4];
    uint32_t size;
    char type[4];
};

DatatableReader::DatatableReader(vector<char>&& input)
    : input_(move(input))
    , current_row_(0)
{
    ValidateFile(input_);

    uint32_t data_position = sizeof(IffHeader) * 2; // Starts after the second iff header    
    column_header_ = reinterpret_cast<const ColumnHeader*>(&input_[data_position]);
    column_offset_ = &input_[data_position + sizeof(ColumnHeader)];

    ParseColumnNames();

    data_position += sizeof(ColumnHeader) - sizeof(uint32_t) + utils::bigToHost(column_header_->size);
    type_header_ = reinterpret_cast<const TypeHeader*>(&input_[data_position]);
    type_offset_ =  &input_[data_position + sizeof(TypeHeader)];
    
    ParseColumnTypes();

    data_position += sizeof(TypeHeader) + utils::bigToHost(type_header_->size);
    row_header_ = reinterpret_cast<const RowHeader*>(&input_[data_position]);
    row_offset_ = &input_[data_position + sizeof(RowHeader)];

    ParseRows();
}

uint32_t DatatableReader::CountRows() const
{
    return row_header_->count;
}

const vector<string>& DatatableReader::GetColumnNames() const
{
    return column_names_;
}

bool DatatableReader::Next()
{
    ++current_row_;

    return current_row_ < row_header_->count;
}

std::map<std::string, DatatableCell*> DatatableReader::GetRow()
{
    if (current_row_ >= row_header_->count)
    {
        throw out_of_range("Accessed past the end of the rows");
    }

    std::map<string, DatatableCell*> row;

    transform(
        begin(column_names_), end(column_names_),
        row_data_[current_row_].begin(), inserter(row, row.end()),
        [] (string name, DatatableCell& value)
    {
        return make_pair(name, &value);
    });

    return row;
}

void DatatableReader::ValidateFile(const vector<char>& input) const
{
    const IffHeader* header = reinterpret_cast<const IffHeader*>(&input[0]);
    
    if (string(header->type, 4).compare("DTII") != 0)
    {
        throw runtime_error("Invalid datatable file format");
    }
}

void DatatableReader::ParseColumnNames()
{
    string tmp;
    for (uint32_t i = 0, offset = 0; i < column_header_->count; ++i)
    {
        tmp = string(column_offset_ + offset);
        column_names_.push_back(tmp);
        offset += tmp.size() + 1;
    }
}

void DatatableReader::ParseColumnTypes()
{
    string tmp;
    for (uint32_t i = 0, offset = 0; i < column_header_->count; ++i)
    {
        tmp = string(type_offset_ + offset);
        column_types_.push_back(tmp);
        offset += tmp.size() + 1;
    }
}

void DatatableReader::ParseRows()
{
    for (uint32_t i = 0, offset = 0; i < row_header_->count; ++i)
    {
        vector<DatatableCell> row;

        for_each(
            begin(column_types_),
            end(column_types_),
            [this, &row, &offset] (const string& type) 
        {
            offset += ParseColumn(static_cast<uint8_t>(type[0]), offset, row);
        });

        row_data_.push_back(move(row));
    }
}


uint32_t DatatableReader::ParseColumn(uint8_t type, uint32_t offset, vector<DatatableCell>& row)
{
    uint32_t size = 0;
    DatatableCell cell;

    switch(type)
    {
    case 'f':
        cell.SetValue(reinterpret_cast<const float*>(row_offset_ + offset));

        size = 4;
        break;
        
    case 'b':
    case 'h':
    case 'e':
    case 'i':
        cell.SetValue(reinterpret_cast<const int*>(row_offset_ + offset));

        size = 4;
        break;

    case 's':
        cell.SetValue(row_offset_ + offset);
        
        string tmp(row_offset_ + offset);
        size = tmp.length() + 1;
        break;
    }    

    row.push_back(move(cell));

    return size;
}

string DatatableCell::ToString() const
{
    string str;

    try 
    {
        if (value_.type() == typeid(const int*))
        {
            str = boost::lexical_cast<string>(*boost::any_cast<const int*>(value_));
        }  
        else if (value_.type() == typeid(const bool*))
        {
            str = boost::lexical_cast<string>(*boost::any_cast<const bool*>(value_));
        }
        else if (value_.type() == typeid(const float*))
        {
            str = boost::lexical_cast<string>(*boost::any_cast<const float*>(value_));
        }
        else if (value_.type() == typeid(const char*))
        {
            str = boost::any_cast<const char*>(value_);
        }
        else
        {
            str = "";
        }
    }
    catch (...) 
    {
        str = "";
    }

    return str;
}