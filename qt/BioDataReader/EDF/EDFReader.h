#ifndef EDFREADER_H
#define EDFREADER_H

#include <QObject>
#include <stdio.h>
#include <QFile>
#include <QDateTime>

#include "edflib/edflib.h"

struct EDFInfo
{
    bool valid = false;
    QString file = "";
    QString subject = "";
    QDateTime startDate;
    int channels = 0;
    float frequency = 0;
    float durationSeconds = 0;
    QString dimension = "";
};

class EDFReader : public QObject
{
    Q_OBJECT
public:
    explicit EDFReader();
    explicit EDFReader(QString in_path, QObject *in_pParent = nullptr);
    ~EDFReader();
    QString infoShort();
    QString infoFull();
    static EDFInfo fileInfo(QString in_file);

    static QString timeToText(double seconds, bool showMs);

    bool open();
    bool close();

    QStringList getChannelNames();
    QString getChannelName(int channel);
    int getBuffData(uint offset, uint samples, double* out_pData);
    int getChannelBuffData(uint channel, uint offset, uint samples, double* out_pData);
    QVector<double> getChannelData(uint channel, uint offset, uint samples);
    QVector<float> getChannelDataFloat(uint channel, uint offset, uint samples);
    bool isFileOpened() const;
    QMap<QString, QVariant> getPatientInfo();
    int channelAmount() { return m_channelAmount; }
    QString dimension() { return m_dimension; }
    float samplingRate() { return m_samplingRate; }
    void setPath(const QString& in_path);
    QString getPath() { return m_path; }
    double getSeconds() { return m_seconds; }
    unsigned int lengthSamples() { return m_totalSamples; }
    QDateTime startDate() { return m_startDate; }
    void getAnnotationList(QList<edf_annotation_struct>& out_list) const;
    QMap<QString,float> getFilters() { return m_filters; }
protected:
    template<typename T> QVector<T> getChannelDataTemplate(uint channel, uint offset, uint samples);

//    Montage_ptr m_current_montage;
    QDateTime m_startDate;    
    int m_channelAmount = 0;
    float m_samplingRate = 0;
    QString m_dimension = "";
    QList<edf_annotation_struct> m_annotations;
    edf_hdr_struct m_hdr;
    QString m_path;
    QString m_patientName;
    QString m_patientData;
    QString m_patientGender;
    QString m_equipmentInfo;
    int m_currHandle;
    int m_sampleRecord = 0;
    double m_seconds = 0;
    unsigned int m_totalSamples = 0;
    bool m_isOpened = false;
    QVector<double> m_temp_buffer;
    QMap<QString,float> m_filters;

    QStringList m_channelNames;

    bool internalOpen();
    QString getPatientData();
    QString getPatientName();
    QString getPatientGender();
    QString getEquipmentInfo();
    bool takeCurrentAnnotations();
};

#endif // EDFREADER_H
