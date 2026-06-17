#ifndef CONFERENCE_H
#define CONFERENCE_H

#include "Source.h"

class Conference : public Source {
private:
    std::string m_meetingAddress;
public:
    std::string getType() const override { return "Conference"; }
    const std::string& getMeetingAddress() const { return m_meetingAddress; }
    void setMeetingAddress(const std::string& addr) { m_meetingAddress = addr; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // CONFERENCE_H
