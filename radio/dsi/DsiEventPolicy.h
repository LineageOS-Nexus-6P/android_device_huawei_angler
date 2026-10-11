#pragma once

namespace angler::dsi {

constexpr int kDsiEventNetIsConnected = 1;
constexpr int kDsiEventNetNoNet = 2;
constexpr int kDsiEventWdsConnected = 18;

enum class EventAction {
    kForward,
    kDrop,
    kTranslateToNetIsConnected,
};

constexpr bool deferredConnectionHasEvidence(int handleAddressCount,
                                             int sharedBaselineAddressCount,
                                             int sharedAddressCount) {
    if (handleAddressCount > 0) {
        return true;
    }

    return sharedBaselineAddressCount > 0
            && sharedAddressCount > sharedBaselineAddressCount;
}

constexpr EventAction decideEventAction(bool connectedDelivered,
                                        unsigned int /*addressCount*/,
                                        int event) {
    if (event == kDsiEventNetIsConnected) {
        return connectedDelivered ? EventAction::kDrop : EventAction::kForward;
    }

    if (event != kDsiEventWdsConnected) {
        return EventAction::kForward;
    }

    if (!connectedDelivered) {
        return EventAction::kTranslateToNetIsConnected;
    }

    return EventAction::kDrop;
}

}  // namespace angler::dsi
