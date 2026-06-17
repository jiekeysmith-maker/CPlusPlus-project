#ifndef SOURCE_H
#define SOURCE_H

#include "CoreTypes.h"
#include "Serializable.h"

#include <string>

class Source : public Serializable {
protected:
    IdType m_id;
    std::string m_shortName;
    std::string m_fullName;
    std::string m_field;
    std::string m_publisher;
    std::string m_publisherAddress;
    std::string m_retrievalType;
    std::string m_remark;

public:
    Source() : m_id(INVALID_ID) {}
    virtual ~Source() = default;

    virtual std::string getType() const = 0;

    IdType getId() const { return m_id; }
    void setId(IdType id) { m_id = id; }
    const std::string& getShortName() const { return m_shortName; }
    void setShortName(const std::string& name) { m_shortName = name; }
    const std::string& getFullName() const { return m_fullName; }
    void setFullName(const std::string& name) { m_fullName = name; }
    const std::string& getField() const { return m_field; }
    void setField(const std::string& field) { m_field = field; }
    const std::string& getPublisher() const { return m_publisher; }
    void setPublisher(const std::string& pub) { m_publisher = pub; }
    const std::string& getPublisherAddress() const { return m_publisherAddress; }
    void setPublisherAddress(const std::string& addr) { m_publisherAddress = addr; }
    const std::string& getRetrievalType() const { return m_retrievalType; }
    void setRetrievalType(const std::string& type) { m_retrievalType = type; }
    const std::string& getRemark() const { return m_remark; }
    void setRemark(const std::string& remark) { m_remark = remark; }
};

#endif // SOURCE_H
