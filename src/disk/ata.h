#ifndef ATA_H
#define ATA_H

#include <util.h>
#include <stdint.h>
#include <stddef.h>
#include <devices.h>

#define PRIMARY_ATA_IRQ 14
#define SECONDARY_ATA_IRQ 15

#define MAX_ATA_DRIVES 8

typedef struct PACKED ATA_Identify_Device_Data {
    struct {
        uint16_t reserved : 2;
        uint16_t responseIncomplete : 1;
        uint16_t reserved2 : 3;
        uint16_t fixedDevice : 1;
        uint16_t removableMedia : 1;
        uint16_t reserved3 : 7;
        uint16_t deviceType : 1;
    } PACKED generalConfig;
    uint16_t numCylinders;          // Number of cylinders
    uint16_t specificConfig;        // Specific configuration (not specified)
    uint16_t numHeads;              // Number of heads
    uint16_t _reserved0[2];         // Reserved
    uint16_t numSectorsPerTrack;    // Number of sectors per track
    uint16_t vendorUnique[3];       // Vendor-specific information
    char serialNumber[20];          // Serial number
    uint16_t _reserved2[3];         // Reserved
    char firmwareRevision[8];       // Firmware revision
    char modelNumber[40];           // Model number
    uint8_t maxBlockTransfer;       // Maximum block transfer
    uint8_t vendorUnique2;          // Vendor-specific information
    struct {
        uint16_t freatureSupported : 1;
        uint16_t reserved : 15;
    } PACKED trustedComputing;
    struct {
        uint8_t currentLongPhysicalSectorAssignment : 2;
        uint8_t reserved : 6;
        uint8_t dmaSupported : 1;
        uint8_t lbaSupported : 1;
        uint8_t iordyDisable : 1;
        uint8_t iordySupported : 1;
        uint8_t reserved2 : 1;
        uint8_t standbyTimerSupport : 1;
        uint8_t reserved3 : 2;
        uint16_t reserved4;
    } PACKED capabilities;
    uint16_t _reserved3[2];         // Reserved
    uint16_t translationFieldsValid : 3;
    uint16_t _reserved4 : 5;
    uint16_t freeFallControlSensitivity : 8;
    uint16_t numCurrentCylinders;
    uint16_t numCurrentHeads;
    uint16_t numCurrentSectorsPerTrack;
    uint32_t currentSectorCapacity;           // Current sector capacity (what does this mean?)
    uint8_t currentMultiSectorSetting;
    uint8_t multiSectorSettingValid : 1;
    uint8_t _reserved5 : 3;
    uint8_t sanitizeFeatureSupported: 1;
    uint8_t cryptoScrambleSupported : 1;
    uint8_t overwriteExtSupported : 1;
    uint8_t blockEraseExtSupported : 1;
    uint32_t addressableSectors;
    uint16_t _reserved6;
    uint16_t multiWordDmaSupport : 8;
    uint16_t multiWordDmaActive : 8;
    uint16_t advancedPioModes : 8;
    uint16_t _reserved7 : 8;
    uint16_t minMultiWordDmaCycleTime;
    uint16_t recommendedMultiWordDmaCycleTime;
    uint16_t minPioCycleTime;
    uint16_t minPioCycleTimeIORDY;
    struct {
        uint16_t zonedCapabilities : 2;
        uint16_t nvWriteCache : 1;
        uint16_t extendedUserAddressableSectorsSupported : 1;
        uint16_t deviceEncryptsAllData : 1;
        uint16_t readZeroAfterTrimSupported : 1;
        uint16_t optional28BitCommandsSupported : 1;
        uint16_t IEEE1667Compliant : 1;
        uint16_t downloadMicrocode : 1;
        uint16_t setMaxSetPasswordUnlockDmaSupported : 1;
        uint16_t writeBufferDmaSupported : 1;
        uint16_t readBufferDmaSupported : 1;
        uint16_t deviceConfigIdentifySetDmaSupported : 1;
        uint16_t LPSAERCSupported : 1;
        uint16_t deterministicReadAfterTrimSupported : 1;
        uint16_t cFastSpecSupported : 1;
    } PACKED additionalSupported;
    uint16_t _reserved8[5];        // Reserved
    uint16_t queueDepth : 5;
    uint16_t _reserved9 : 11;
    struct {
        uint16_t reserved0 : 1;
        uint16_t sataGen1 : 1;
        uint16_t sataGen2 : 1;
        uint16_t sataGen3 : 1;
        uint16_t reserved1 : 4;
        uint16_t NCQ : 1;
        uint16_t HIPM : 1;
        uint16_t phyEvents : 1;
        uint16_t ncqUnload : 1;
        uint16_t ncqPriority : 1;
        uint16_t hostAutoPS : 1;
        uint16_t deviceAutoPS : 1;
        uint16_t readLogDMA : 1;
        uint16_t reserved2 : 1;
        uint16_t currentSpeed : 3;
        uint16_t ncqStreaming : 1;
        uint16_t ncqQueueManagement : 1;
        uint16_t ncqRecieveSend : 1;
        uint16_t DEVSLPtoReducePower : 1;
        uint16_t reserved3 : 8;
    } PACKED sataCapabilities;
    struct {
        uint16_t Reserved0 : 1;
        uint16_t NonZeroOffsets : 1;
        uint16_t DmaSetupAutoActivate : 1;
        uint16_t DIPM : 1;
        uint16_t InOrderData : 1;
        uint16_t HardwareFeatureControl : 1;
        uint16_t SoftwareSettingsPreservation : 1;
        uint16_t NCQAutosense : 1;
        uint16_t DEVSLP : 1;
        uint16_t HybridInformation : 1;
        uint16_t Reserved1 : 6;
    } PACKED sataFeaturesSupported;
    struct {
        uint16_t reserved0 : 1;
        uint16_t nonZeroOffsets : 1;
        uint16_t dmaSetupAutoActivate : 1;
        uint16_t DIPM : 1;
        uint16_t inOrderData : 1;
        uint16_t hardwareFeatureControl : 1;
        uint16_t softwareSettingsPreservation : 1;
        uint16_t deviceAutoPS : 1;
        uint16_t DEVSLP : 1;
        uint16_t hybridInformation : 1;
        uint16_t reserved1 : 6;
    } PACKED sataFeaturesEnabled;
    uint16_t revisionMajor;
    uint16_t revisionMinor;
    struct {
        uint16_t SmartCommands : 1;
        uint16_t SecurityMode : 1;
        uint16_t RemovableMediaFeature : 1;
        uint16_t PowerManagement : 1;
        uint16_t Reserved1 : 1;
        uint16_t WriteCache : 1;
        uint16_t LookAhead : 1;
        uint16_t ReleaseInterrupt : 1;
        uint16_t ServiceInterrupt : 1;
        uint16_t DeviceReset : 1;
        uint16_t HostProtectedArea : 1;
        uint16_t Obsolete1 : 1;
        uint16_t WriteBuffer : 1;
        uint16_t ReadBuffer : 1;
        uint16_t Nop : 1;
        uint16_t Obsolete2 : 1;
        uint16_t DownloadMicrocode : 1;
        uint16_t DmaQueued : 1;
        uint16_t Cfa : 1;
        uint16_t AdvancedPm : 1;
        uint16_t Msn : 1;
        uint16_t PowerUpInStandby : 1;
        uint16_t ManualPowerUp : 1;
        uint16_t Reserved2 : 1;
        uint16_t SetMax : 1;
        uint16_t Acoustics : 1;
        uint16_t BigLba : 1;
        uint16_t DeviceConfigOverlay : 1;
        uint16_t FlushCache : 1;
        uint16_t FlushCacheExt : 1;
        uint16_t WordValid83 : 2;
        uint16_t SmartErrorLog : 1;
        uint16_t SmartSelfTest : 1;
        uint16_t MediaSerialNumber : 1;
        uint16_t MediaCardPassThrough : 1;
        uint16_t StreamingFeature : 1;
        uint16_t GpLogging : 1;
        uint16_t WriteFua : 1;
        uint16_t WriteQueuedFua : 1;
        uint16_t WWN64Bit : 1;
        uint16_t URGReadStream : 1;
        uint16_t URGWriteStream : 1;
        uint16_t ReservedForTechReport : 2;
        uint16_t IdleWithUnloadFeature : 1;
        uint16_t WordValid : 2;
    } PACKED commandSetSupport;
    struct {
        uint16_t SmartCommands : 1;
        uint16_t SecurityMode : 1;
        uint16_t RemovableMediaFeature : 1;
        uint16_t PowerManagement : 1;
        uint16_t Reserved1 : 1;
        uint16_t WriteCache : 1;
        uint16_t LookAhead : 1;
        uint16_t ReleaseInterrupt : 1;
        uint16_t ServiceInterrupt : 1;
        uint16_t DeviceReset : 1;
        uint16_t HostProtectedArea : 1;
        uint16_t Obsolete1 : 1;
        uint16_t WriteBuffer : 1;
        uint16_t ReadBuffer : 1;
        uint16_t Nop : 1;
        uint16_t Obsolete2 : 1;
        uint16_t DownloadMicrocode : 1;
        uint16_t DmaQueued : 1;
        uint16_t Cfa : 1;
        uint16_t AdvancedPm : 1;
        uint16_t Msn : 1;
        uint16_t PowerUpInStandby : 1;
        uint16_t ManualPowerUp : 1;
        uint16_t Reserved2 : 1;
        uint16_t SetMax : 1;
        uint16_t Acoustics : 1;
        uint16_t BigLba : 1;
        uint16_t DeviceConfigOverlay : 1;
        uint16_t FlushCache : 1;
        uint16_t FlushCacheExt : 1;
        uint16_t Resrved3 : 1;
        uint16_t Words119_120Valid : 1;
        uint16_t SmartErrorLog : 1;
        uint16_t SmartSelfTest : 1;
        uint16_t MediaSerialNumber : 1;
        uint16_t MediaCardPassThrough : 1;
        uint16_t StreamingFeature : 1;
        uint16_t GpLogging : 1;
        uint16_t WriteFua : 1;
        uint16_t WriteQueuedFua : 1;
        uint16_t WWN64Bit : 1;
        uint16_t URGReadStream : 1;
        uint16_t URGWriteStream : 1;
        uint16_t ReservedForTechReport : 2;
        uint16_t IdleWithUnloadFeature : 1;
        uint16_t Reserved4 : 2;
    } PACKED commandSetActive;
    uint16_t udmaSupport : 8;
    uint16_t udmaActive : 8;
    struct {
        uint16_t timeRequiredToSpinUp : 15;
        uint16_t extendedTimeReported : 1;
    } PACKED normalSecurityEraseUnit;
    struct {
        uint16_t timeRequiredToSpinUp : 15;
        uint16_t extendedTimeReported : 1;
    } PACKED enhancedSecurityEraseUnit;
    uint16_t CurrentAPMLevel : 8;
    uint16_t ReservedWord91 : 8;
    uint16_t MasterPasswordID;
    uint16_t HardwareResetResult;
    uint16_t CurrentAcousticValue : 8;
    uint16_t RecommendedAcousticValue : 8;
    uint16_t StreamMinRequestSize;
    uint16_t StreamingTransferTimeDMA;
    uint16_t StreamingAccessLatencyDMAPIO;
    uint32_t  StreamingPerfGranularity;
    uint32_t  Max48BitLBA[2];
    uint16_t StreamingTransferTime;
    uint16_t DsmCap;
    struct {
        uint16_t LogicalSectorsPerPhysicalSector : 4;
        uint16_t Reserved0 : 8;
        uint16_t LogicalSectorLongerThan256Words : 1;
        uint16_t MultipleLogicalSectorsPerPhysicalSector : 1;
        uint16_t Reserved1 : 2;
    } PACKED PhysicalLogicalSectorSize;
    uint16_t InterSeekDelay;
    uint16_t WorldWideName[4];
    uint16_t ReservedForWorldWideName128[4];
    uint16_t ReservedForTlcTechnicalReport;
    uint16_t WordsPerLogicalSector[2];
    struct {
        uint16_t ReservedForDrqTechnicalReport : 1;
        uint16_t WriteReadVerify : 1;
        uint16_t WriteUncorrectableExt : 1;
        uint16_t ReadWriteLogDmaExt : 1;
        uint16_t DownloadMicrocodeMode3 : 1;
        uint16_t FreefallControl : 1;
        uint16_t SenseDataReporting : 1;
        uint16_t ExtendedPowerConditions : 1;
        uint16_t Reserved0 : 6;
        uint16_t WordValid : 2;
    } PACKED CommandSetSupportExt;
    struct {
        uint16_t ReservedForDrqTechnicalReport : 1;
        uint16_t WriteReadVerify : 1;
        uint16_t WriteUncorrectableExt : 1;
        uint16_t ReadWriteLogDmaExt : 1;
        uint16_t DownloadMicrocodeMode3 : 1;
        uint16_t FreefallControl : 1;
        uint16_t SenseDataReporting : 1;
        uint16_t ExtendedPowerConditions : 1;
        uint16_t Reserved0 : 6;
        uint16_t Reserved1 : 2;
    } PACKED CommandSetActiveExt;
    uint16_t ReservedForExpandedSupportandActive[6];
    uint16_t MsnSupport : 2;
    uint16_t ReservedWord127 : 14;
    struct {
        uint16_t SecuritySupported : 1;
        uint16_t SecurityEnabled : 1;
        uint16_t SecurityLocked : 1;
        uint16_t SecurityFrozen : 1;
        uint16_t SecurityCountExpired : 1;
        uint16_t EnhancedSecurityEraseSupported : 1;
        uint16_t Reserved0 : 2;
        uint16_t SecurityLevel : 1;
        uint16_t Reserved1 : 7;
    } SecurityStatus;
    uint16_t ReservedWord129[31];
    struct {
        uint16_t MaximumCurrentInMA : 12;
        uint16_t CfaPowerMode1Disabled : 1;
        uint16_t CfaPowerMode1Required : 1;
        uint16_t Reserved0 : 1;
        uint16_t Word160Supported : 1;
    } PACKED CfaPowerMode1;
    uint16_t ReservedForCfaWord161[7];
    uint16_t NominalFormFactor : 4;
    uint16_t ReservedWord168 : 12;
    struct {
        uint16_t SupportsTrim : 1;
        uint16_t Reserved0 : 15;
    } PACKED DataSetManagementFeature;
    uint16_t AdditionalProductID[4];
    uint16_t ReservedForCfaWord174[2];
    uint16_t CurrentMediaSerialNumber[30];
    struct {
        uint16_t Supported : 1;
        uint16_t Reserved0 : 1;
        uint16_t WriteSameSuported : 1;
        uint16_t ErrorRecoveryControlSupported : 1;
        uint16_t FeatureControlSuported : 1;
        uint16_t DataTablesSuported : 1;
        uint16_t Reserved1 : 6;
        uint16_t VendorSpecific : 4;
    } PACKED SCTCommandTransport;
    uint16_t ReservedWord207[2];
    struct {
        uint16_t AlignmentOfLogicalWithinPhysical : 14;
        uint16_t Word209Supported : 1;
        uint16_t Reserved0 : 1;
    } PACKED BlockAlignment;
    uint16_t WriteReadVerifySectorCountMode3Only[2];
    uint16_t WriteReadVerifySectorCountMode2Only[2];
    struct {
        uint16_t NVCachePowerModeEnabled : 1;
        uint16_t Reserved0 : 3;
        uint16_t NVCacheFeatureSetEnabled : 1;
        uint16_t Reserved1 : 3;
        uint16_t NVCachePowerModeVersion : 4;
        uint16_t NVCacheFeatureSetVersion : 4;
    } PACKED NVCacheCapabilities;
    uint16_t NVCacheSizeLSW;
    uint16_t NVCacheSizeMSW;
    uint16_t NominalMediaRotationRate;
    uint16_t ReservedWord218;
    struct {
        uint8_t NVCacheEstimatedTimeToSpinUpInSeconds;
        uint8_t Reserved;
    } PACKED NVCacheOptions;
    uint16_t WriteReadVerifySectorCountMode : 8;
    uint16_t ReservedWord220 : 8;
    uint16_t ReservedWord221;
    struct {
        uint16_t MajorVersion : 12;
        uint16_t TransportType : 4;
    } PACKED TransportMajorVersion;
    uint16_t TransportMinorVersion;
    uint16_t ReservedWord224[6];
    uint32_t  ExtendedNumberOfUserAddressableSectors[2];
    uint16_t MinBlocksPerDownloadMicrocodeMode03;
    uint16_t MaxBlocksPerDownloadMicrocodeMode03;
    uint16_t ReservedWord236[19];
    uint16_t Signature : 8;
    uint16_t CheckSum : 8;
} PACKED ataInfo_t;

int InitializeAta();

#endif