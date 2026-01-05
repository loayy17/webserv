#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP
class PollManager {
   private:

    PollManager(const PollManager&);
    PollManager& operator=(const PollManager&);

   public:
    PollManager();
    ~PollManager();
};

#endif