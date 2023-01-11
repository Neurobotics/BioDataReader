#include "EDFReader.h"
#include <QDebug>

using namespace std;

QString PrintLikeTable(QMap<QString, QString> values, QString headerSeparator)
{
    if (values.count() == 0) return "";

    QStringList keys = values.keys();
    int headerLength = 0;

    for (int i = 0, sz = keys.count(); i < sz; i++)
    {
        QString key = keys[i];
        int len = key.length();
        if (headerLength < len)
            headerLength = len;
    }

    headerLength ++;

    QString result = "";
    for (int i = 0, sz = keys.count(); i < sz; i++)
    {
        QString key = keys[i];
        int len = key.length() + headerSeparator.length();
        result.append(key + headerSeparator);
        while (len <= headerLength)
        {
            result.append(" ");
            len++;
        }
        result.append(values[key] + "\r\n");
    }
    return result.trimmed();
}



EDFReader::EDFReader()
{
    m_sampleRecord = 0;
    m_channelAmount = 0;
}

EDFReader::EDFReader(QString in_path, QObject *in_pParent) : QObject(in_pParent)
{
    m_path = in_path;
    m_sampleRecord = 0;
    m_channelAmount = 0;
}

EDFReader::~EDFReader()
{
    close();
}

QMap<QString, QVariant> EDFReader::getPatientInfo()
{
    QMap<QString, QVariant> map;
    if (isFileOpened())
    {
        map.insert("name", this->getPatientName());
        map.insert("equipment", this->getEquipmentInfo());
        map.insert("gender", this->getPatientGender()[0] == 'M' ? 0 : 1);
    }
    return map;
}

QString EDFReader::infoShort()
{
    return timeToText(qMax(0.0, getSeconds()), false) + ", " + QString::number(channelAmount())
            + "ch, " + QString::number(qMax(0.0, static_cast<double>(samplingRate())), 'f', 2) + tr("Hz");
}

QString EDFReader::infoFull()
{
    QMap<QString, QString> inf;
    inf[tr("Duration")] = timeToText(getSeconds(), false);
    inf[tr("Channels")] = QString::number(channelAmount());
    inf[tr("Sampling rate")] = QString::number(static_cast<double>(samplingRate()), 'f', 2) + tr("Hz");
    inf[tr("Subject") ] = m_patientName;
    inf[tr("Start time")] = m_startDate.toString("yyyy/MM/dd hh:mm:ss");
    return PrintLikeTable(inf, ":");
}

EDFInfo EDFReader::fileInfo(QString in_file)
{
    EDFInfo res;
    res.valid = QFile::exists(in_file);
    res.file = in_file;
    if (res.valid)
    {
        EDFReader r(in_file);
        if (r.open())
        {
            res.valid = true;
            res.frequency = r.samplingRate();
            res.durationSeconds = static_cast<float>(r.getSeconds());
            res.startDate = r.startDate();
            res.channels = r.channelAmount();
            res.dimension = r.dimension();
            r.close();
        }
    }
    return res;
}

bool EDFReader::open()
{
    m_isOpened = internalOpen();
    return m_isOpened;
}

QString EDFReader::getPatientGender()
{
    return m_patientGender;
}

QString EDFReader::getPatientName()
{
    return m_patientName;
}

QString EDFReader::getEquipmentInfo()
{
    return m_equipmentInfo;
}

QString EDFReader::getPatientData()
{
    return m_patientData;
}

void EDFReader::setPath(const QString& in_path)
{
    m_path = in_path;
}

QVector<float> EDFReader::getChannelDataFloat(uint channel, uint offset, uint samples)
{
    return getChannelDataTemplate<float>(channel, offset, samples);
}

QVector<double> EDFReader::getChannelData(uint channel, uint offset, uint samples)
{
    return getChannelDataTemplate<double>(channel, offset, samples);
}

bool EDFReader::internalOpen()
{    
    if (!QFile::exists(m_path))
    {
        return false;
    }

    QByteArray ba = m_path.toLocal8Bit();

    int error_code = 0;
    if ((error_code = edfopen_file_readonly(ba.constData(), &m_hdr, EDFLIB_READ_ALL_ANNOTATIONS)))
    {

        switch(m_hdr.filetype)
        {
        case EDFLIB_MALLOC_ERROR                : qDebug() << "malloc error";  return false;
        case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY   : qDebug() << "\ncan not open file, no such file or directory";  return false;
        case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS : qDebug() << "the file is not EDF(+) or BDF(+) compliant\r\n(it contains format errors)";  return false;
        case EDFLIB_MAXFILES_REACHED            : qDebug() << "to many files opened";  return false;
        case EDFLIB_FILE_READ_ERROR             : qDebug() << "a read error occurred";  return false;
        case EDFLIB_FILE_ALREADY_OPENED         : qDebug() << "file has already been opened";  return false;
        default                                 : qDebug() << "unknown error";
            break;
        }
    }
    m_startDate = QDateTime(QDate(m_hdr.startdate_year, m_hdr.startdate_month, m_hdr.startdate_day),
                            QTime(m_hdr.starttime_hour, m_hdr.starttime_minute, m_hdr.starttime_second, static_cast<int>(m_hdr.starttime_subsecond)));

    m_currHandle = m_hdr.handle;
    m_channelAmount = m_hdr.edfsignals;
    m_seconds = m_hdr.file_duration/EDFLIB_TIME_DIMENSION;
    m_sampleRecord = m_hdr.signalparam[0].smp_in_datarecord;

    m_patientName = m_hdr.patient;
    m_patientData = m_hdr.birthdate;
    m_patientGender = m_hdr.gender;
    m_equipmentInfo = m_hdr.equipment;

    m_channelNames.clear();

    QString filters = m_hdr.signalparam[0].prefilter;

    for(QString item: filters.split(" "))
    {
        if(!item.isEmpty())
        {
            item.chop(2);
            m_filters[item.left(2)] = item.section(":",1,1).toFloat();
        }
    }

    if (m_channelAmount > 0)
    {
        m_dimension = QString(m_hdr.signalparam[0].physdimension).trimmed();

        m_totalSamples = static_cast<unsigned int>(m_hdr.signalparam[0].smp_in_file);
        m_samplingRate = (static_cast<float>(m_hdr.signalparam[0].smp_in_datarecord) /
                static_cast<float>(m_hdr.datarecord_duration)) * EDFLIB_TIME_DIMENSION;

        for (int i = 0; i < m_channelAmount; i++)
        {
            m_channelNames << QString(m_hdr.signalparam[i].label).trimmed();
        }

        takeCurrentAnnotations();
    }
    return true;
}

QString EDFReader::timeToText(double seconds, bool showMs)
{
    int secs = seconds;

    int ms = (seconds - secs)*1000;

    secs = secs%60;
    int minutes = seconds/60;
    int hours = minutes/60;

    QString str = QString("%1").arg(minutes%60, 2, 10, QChar('0')) + ":" + QString("%1").arg(secs, 2, 10, QChar('0'));

    if (showMs)
        str += "." + QString("%1").arg(ms, 3, 10, QChar('0'));

    if (hours > 0)
        str = QString("%1").arg(hours, 2, 10, QChar('0')) + ":" + str;

    return str;
}

bool EDFReader::takeCurrentAnnotations()
{
    int index = 0;
    while(true)
    {
        edf_annotation_struct str;
        int res = edf_get_annotation(m_currHandle, index, &str);
        if (index == 0 && res < 0)
        {
            return false;
        }
        else if (res < 0)
            return true;

        m_annotations.push_back(str);
        ++index;
    }
}

void EDFReader::getAnnotationList(QList<edf_annotation_struct>& out_list) const
{
    out_list = m_annotations;
}

bool EDFReader::close()
{
    if (m_isOpened && m_currHandle != -1)
        return edfclose_file(m_currHandle) != -1;

    return false;
}

QStringList EDFReader::getChannelNames()
{
    QStringList names;
    for (uint i = 0; i<m_channelAmount; i++)
    {
        names << getChannelName(i);
    }
    return names;
}

bool EDFReader::isFileOpened() const
{
    return m_isOpened;
}

template<typename T> QVector<T> EDFReader::getChannelDataTemplate(uint channel, uint offset, uint samples)
{
    QVector<T> chdata(samples);
    T* pData = chdata.data();
    if (!m_isOpened || channel > m_channelAmount-1)
        return chdata;

    m_temp_buffer.resize(samples);
    edfseek(m_currHandle, static_cast<int>(channel), static_cast<long long>(offset), EDFSEEK_SET);
    auto res = edfread_physical_samples(m_currHandle, static_cast<int>(channel), static_cast<int>(samples), m_temp_buffer.data());
    if (res == (-1))
    {

    }
    else
    {
        double* pTempData = m_temp_buffer.data();
        for (unsigned int i = 0; i < samples; i++)
            *(pData + i) = static_cast<T>(*(pTempData + i)) * -1;
    }
    return chdata;
}

int EDFReader::getChannelBuffData(uint channel, uint offset, uint samples, double* out_pData)
{
    if (!m_isOpened || channel > m_channelAmount - 1)
    {
        return -2;
    }
    long long seek = edfseek(m_currHandle, static_cast<int>(channel), static_cast<int>(offset), EDFSEEK_SET);
    if (seek < 0)
    {
        return -1;
    }

    int res = edfread_physical_samples(m_currHandle, static_cast<int>(channel), static_cast<int>(samples), out_pData);
    if (res == (-1))
    {
        return -1;
    }
    for (unsigned int i = 0; i < samples; i++)
        *(out_pData + i) = *(out_pData + i) * -1;

    return 0;
}

QString EDFReader::getChannelName(int channel)
{
    if (channel < m_channelNames.length())
        return m_channelNames[channel];
    return "";
}

int EDFReader::getBuffData(uint offset, uint samples, double* out_pData)
{
    if (!m_isOpened)
    {
        return -2;
    }

    for (unsigned int channel = 0; channel < m_channelAmount; channel++)
    {
        int res = getChannelBuffData(channel, offset, samples, out_pData + channel * samples);
        if (res < 0)
        {
            return -1;
        }
    }
    return 0;
}
