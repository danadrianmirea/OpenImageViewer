#include "MessageHelper.h"
#include "MessageFormatter.h"
#include "PixelHelper.h"
#include "../OIVImage/OIVFileImage.h"
#include "../ConfigurationLoader.h"
#include "UnitsHelper.h"
namespace OIV
{
    std::wstring  MessageHelper::ParseImageSource(const OIVBaseImageSharedPtr& image)
    {
        switch (image->GetDescriptor().Source)
        {
        case ImageSource::None:
            return L"none";
        case ImageSource::File:
            return L"file";
        case ImageSource::Clipboard:
            return L"clipboard";
        case ImageSource::InternalText:
            return L"internal text";
        case ImageSource::GeneratedByLib:
            return L"auto generated";
        default:
            return L"unknown";
        }
    }
    
    std::wstring MessageHelper::CreateKeyBindingsMessage()
    {
        using namespace std;
        //string message = DefaultHeaderColor + "Image information\n";

        MessageFormatter::FormatArgs args;
        args.keyColor = MessageFormatter::DefaultKeyColor;
        args.maxLines = 24;
        args.minSpaceFromValue = 3;
        args.spacer = '.';
        args.valueColor = MessageFormatter::DefaultValueColor;
        args.spaceBetweenColumns = 3;
        MessageFormatter::MessagesValues& messageValues = args.messageValues;




        auto keybindingsList = ConfigurationLoader::LoadKeyBindings();
        auto commands = ConfigurationLoader::LoadCommandGroups();

        for (const auto& binding : keybindingsList)
        {
            auto it = std::find_if(commands.begin(), commands.end(), [&](auto& element)->bool
                {
                    return element.commandGroupID == binding.GroupID;
                });

                if (it != commands.end())
                    messageValues.emplace_back(binding.KeyCombinationName, MessageFormatter::ValueObjectList{ it->commandDisplayName });
        }

        return MessageFormatter::FormatMetaText(args);
    }

    std::wstring MessageHelper::GetFileTime(const std::wstring& filePath)
    {
        auto fileTime = std::filesystem::last_write_time(filePath);
        std::chrono::system_clock::time_point systemTime;
        auto osVersion = LLUtils::PlatformUtility::GetOSVersion();
        if (osVersion.major > 10 || (osVersion.major == 10 && osVersion.build >= 15063 /*Version 1703*/))
        {
            // Not sure if it's a MS STL bug, but using clock_cast invokes initialization of timezones information
            // which in turn invokes icu.dll, supported only since windows 10 1703.
            // https://docs.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
            systemTime = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
        }
        else
        {
            auto ticks = fileTime.time_since_epoch().count() - std::filesystem::__std_fs_file_time_epoch_adjustment;
            systemTime = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(ticks));
        }

        auto in_time_t = std::chrono::system_clock::to_time_t(systemTime);
        std::wstringstream ss;
        tm tmDest;
        errno_t errorCode = localtime_s(&tmDest, &in_time_t) != 0;
        if (errorCode != 0)
        {
            using namespace std::string_literals;
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "could not convert time_t to a tm structure, error code: "s + std::to_string(errorCode));
        }
        ss << std::put_time(&tmDest, OIV_TEXT("%Y-%m-%d %X"));
        return ss.str();
    }
    std::wstring  MessageHelper::CreateImageInfoMessage(const OIVBaseImageSharedPtr& image)
    {
        using namespace std;
        wstring message = MessageFormatter::DefaultHeaderColor + L"Image information\n";

        MessageFormatter::FormatArgs args;
        args.keyColor = MessageFormatter::DefaultKeyColor;
        args.maxLines = 24;
        args.minSpaceFromValue = 3;
        args.spacer = '.';
        args.valueColor = L"<textcolor=#ffffff>";
        MessageFormatter::MessagesValues& messageValues = args.messageValues;

        std::shared_ptr<OIVFileImage> fileImage = std::dynamic_pointer_cast<OIVFileImage>(image);

        if (fileImage != nullptr)
        {
            auto const& filePath = std::dynamic_pointer_cast<OIVFileImage>(image)->GetFileName();

            if (image->GetDescriptor().Source != ImageSource::File)
                messageValues.emplace_back("Source", MessageFormatter::ValueObjectList{ ParseImageSource(image) });
            else
                messageValues.emplace_back("File path", MessageFormatter::ValueObjectList{ MessageFormatter::FormatFilePath(filePath) });

            auto fileSize = std::filesystem::file_size(filePath);

            messageValues.emplace_back("File size", MessageFormatter::ValueObjectList{ UnitHelper::FormatUnit(fileSize,UnitType::BinaryDataShort,0,0) });
            messageValues.emplace_back("File date", MessageFormatter::ValueObjectList{ GetFileTime(filePath) });
            auto bitmapSize = image->GetDescriptor().Width * image->GetDescriptor().Height * image->GetDescriptor().Bpp / CHAR_BIT;
            auto compressionRatio = static_cast<double>(bitmapSize) / static_cast<double>(fileSize);
            messageValues.emplace_back("Compression ratio", MessageFormatter::ValueObjectList{ L"1:" ,compressionRatio });
        }

        const auto& texelInfo = IMCodec::GetTexelInfo(static_cast<IMCodec::TexelFormat>(image->GetDescriptor().texelFormat));
        
        messageValues.emplace_back("Width", MessageFormatter::ValueObjectList{ image->GetDescriptor().Width , "px" });
        messageValues.emplace_back("Height", MessageFormatter::ValueObjectList{ image->GetDescriptor().Height , "px" });
        messageValues.emplace_back("bit depth", MessageFormatter::ValueObjectList{ image->GetDescriptor().Bpp , " bpp" });
        messageValues.emplace_back("channels info", MessageFormatter::ValueObjectList{ MessageFormatter::FormatTexelInfo(texelInfo) });
        messageValues.emplace_back("Num sub-images", MessageFormatter::ValueObjectList{ image->GetDescriptor().NumSubImages });
        messageValues.emplace_back("Load time",  MessageFormatter::ValueObjectList{ static_cast<long double>(image->GetDescriptor().LoadTime) , "ms" });
        messageValues.emplace_back("Display time", MessageFormatter::ValueObjectList{ image->GetDescriptor().DisplayTime , "ms" });
        messageValues.emplace_back("Codec used", MessageFormatter::ValueObjectList{ image->GetDescriptor().pluginUsed != nullptr ? image->GetDescriptor().pluginUsed : "N/A" });
        
        auto uniqueValues = PixelHelper::CountUniqueValues(image);
        if (uniqueValues > -1)
            messageValues.emplace_back("Unique values", MessageFormatter::ValueObjectList{ uniqueValues });

        message += L'\n' + MessageFormatter::FormatMetaText(args);
        return message;
    }
}