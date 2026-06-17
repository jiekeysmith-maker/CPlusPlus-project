#ifndef JOURNAL_H
#define JOURNAL_H

#include "Source.h"

class Journal : public Source {
private:
    double m_latestImpactFactor;
public:
    Journal() : m_latestImpactFactor(0.0) {}
    std::string getType() const override { return "Journal"; }
    double getImpactFactor() const { return m_latestImpactFactor; }
    void setImpactFactor(double ifactor) { m_latestImpactFactor = ifactor; }

    std::string serialize() const override;
    void deserialize(const std::string& json) override;
};

#endif // JOURNAL_H
