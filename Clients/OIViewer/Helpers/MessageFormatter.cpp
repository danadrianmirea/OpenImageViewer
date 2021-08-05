#include "MessageFormatter.h"
namespace OIV
{
    std::string MessageFormatter::FormatValueObject(const ValueObjectList& objects)
    {
        std::string result;

        for (const auto& e : objects)
            result += FormatValueObject(e);

        return result;
    }

    std::string MessageFormatter::FormatValueObject(const ValueObject& valueObject)
    {
        switch (valueObject.index())
        {
        case 0:
            return numberFormatWithCommas(std::get<int64_t>(valueObject));
            break;
        case 1:
            return numberFormatWithCommas(std::get<long double>(valueObject));
            break;
        case 2:
            return std::get<std::string>(valueObject);
            break;
        }

        return std::string();
    }

    std::string MessageFormatter::FormatMetaText(FormatArgs args)
    {
        using namespace std;

        struct ColumnInfo
        {
            size_t maxFirstLength;
            size_t maxSecondLength;
        };


        const int totalColumns = static_cast<int>(std::ceil(static_cast<double>(args.messageValues.size()) / args.maxLines));
        std::vector<ColumnInfo> columnInfo(totalColumns);
        int currentcolumn = 0;
        int currentLine = 0;

        for (const auto& [key, value] : args.messageValues)
        {
            auto formattedValue = FormatValueObject(value);

            columnInfo[currentcolumn].maxFirstLength = std::max(columnInfo[currentcolumn].maxFirstLength, key.length());
            columnInfo[currentcolumn].maxSecondLength = std::max(columnInfo[currentcolumn].maxSecondLength, formattedValue.length());
            currentLine++;
            if (currentLine >= args.maxLines)
            {
                currentLine = 0;
                currentcolumn++;
            }
        }

        currentcolumn = 0;
        currentLine = 0;



        vector<std::string> lines(std::min<int>(args.messageValues.size(), args.maxLines));


        for (const auto& [key, value] : args.messageValues)
        {
            stringstream ss;
            ss << args.keyColor << key;

            auto formattedValue = FormatValueObject(value);

            size_t currentLength = key.length();
            while (currentLength++ < columnInfo[currentcolumn].maxFirstLength)
                ss << args.spacer;

            for (int i = 0; i < args.minSpaceFromValue - 1; i++)
                ss << args.spacer;

            ss << ' ';

            ss << args.valueColor << formattedValue;

            if (currentcolumn < totalColumns - 1)
            {
                size_t currentLength = formattedValue.length();
                while (currentLength++ < columnInfo[currentcolumn].maxSecondLength)
                    ss << " ";


                auto halfColumSpace = args.spaceBetweenColumns / 2;

                for (int i = 0; i < halfColumSpace; i++)
                    ss << ' ';
                
                ss << "<textcolor=#444444>" << args.columnsSeperator;
                for (int i = halfColumSpace; i < args.spaceBetweenColumns - 1; i++)
                    ss << ' ';
            }

            lines[currentLine] += ss.str();

            currentLine++;
            if (currentLine >= args.maxLines)
            {
                currentLine = 0;
                currentcolumn++;
            }
        }

        stringstream ss1;
        for (const std::string& line : lines)
            ss1 << line << "\n";

        std::string message = ss1.str();
        if (message[message.size() - 1] == '\n')
            message.erase(message.size() - 1);


        return message;
    }


    const std::string& MessageFormatter::PickColor(IMCodec::ChannelSemantic semantic)
    {
        using namespace IMCodec;
        const static std::string blue = "<textcolor=#006dff>";
        const static std::string green = "<textcolor=#00ff00>";
        const static std::string red = "<textcolor=#ff1c21>";
        const static std::string white = "<textcolor=#ffffff>";
        const static std::string other = "<textcolor=#ff8930>";


        switch (semantic)
        {
        case ChannelSemantic::Red:
            return red;
        case ChannelSemantic::Green:
            return green;
        case ChannelSemantic::Blue:
            return blue;
        case ChannelSemantic::Opacity:
            return white;
        case ChannelSemantic::Monochrome:
        case ChannelSemantic::Float:
            return other;
            break;
        case ChannelSemantic::None:
            LL_EXCEPTION_UNEXPECTED_VALUE;
            
            
        }
    }


    const char* MessageFormatter::FormatSemantic(IMCodec::ChannelSemantic semantic)
    {
        switch (semantic)
        {
        case IMCodec::ChannelSemantic::Red:
            return "R";
        case IMCodec::ChannelSemantic::Green:
            return "G";
        case IMCodec::ChannelSemantic::Blue:
            return "B";
        case IMCodec::ChannelSemantic::Opacity:
            return "A";
            break;
        case IMCodec::ChannelSemantic::Monochrome:
            return "Monochrome";
        case IMCodec::ChannelSemantic::Float:
            return "Float";
        case IMCodec::ChannelSemantic::None:
            return "Undefined";
        }
    }

    const char* MessageFormatter::FormatDataType(IMCodec::ChannelDataType dataType)
    {
        switch (dataType)
        {
        case IMCodec::ChannelDataType::Float:
            return "float";
        case IMCodec::ChannelDataType::SignedInt:
            return "signed";
        case IMCodec::ChannelDataType::UnsignedInt:
            return "unsigned";
        case IMCodec::ChannelDataType::None:
            return "undefined";
        }
    }



    std::string MessageFormatter::FormatTexelInfo(const IMCodec::TexelInfo& texelInfo)
    {
        std::stringstream ss;
        using namespace IMCodec;
        bool sameDataTypeForAllchannels = true;

        ChannelSemantic semantic = texelInfo.channles[0].semantic;
        for (size_t i = 1; i < texelInfo.numChannles; i++)
        {
            if (texelInfo.channles[i].semantic != semantic)
            {
                sameDataTypeForAllchannels = true;
                break;
            }
        }

        

        for (size_t i = 0; i < texelInfo.numChannles; i++)
        {
            ss << PickColor(texelInfo.channles[i].semantic) << FormatSemantic(texelInfo.channles[i].semantic) << ':';
            
            if (sameDataTypeForAllchannels == false 
                || texelInfo.channles[i].semantic == ChannelSemantic::Monochrome
                //|| texelInfo.channles[i].semantic == ChannelSemantic::Float
                )
                ss << '(' << FormatDataType(texelInfo.channles[i].ChannelDataType) << ')';

            ss << static_cast<int>(texelInfo.channles[i].width) << " ";
        }

        std::string str = ss.str();

        if (str.empty() == false)
            str.erase(str.size() - 1);

        return ss.str();
    }


    template<class T>
    std::string MessageFormatter::numberFormatWithCommas(T value) {
        struct Numpunct : public std::numpunct<char> {
        protected:
            virtual char do_thousands_sep() const override { return ','; }
            virtual std::string do_grouping() const override { return "\03"; }
        };

        struct StringStreamWrapper
        {
            std::stringstream ss;
            StringStreamWrapper() { ss.imbue({ std::locale(), new Numpunct }); }
        } thread_local ssWrapper;
        auto& ss = ssWrapper.ss;

        ss.str(std::string());
        ss << std::setprecision(2) << std::fixed << value;
        return ss.str();
    }

}