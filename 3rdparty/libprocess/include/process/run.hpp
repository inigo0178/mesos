#ifndef __PROCESS_RUN_HPP__
#define __PROCESS_RUN_HPP__

#include <memory> // TODO(benh): Replace shared_ptr with unique_ptr.

#include <process/process.hpp>

#include <stout/lambda.hpp>
#include <stout/preprocessor.hpp>

namespace process {

namespace internal {

template <typename R>
class ThunkProcess : public Process<ThunkProcess<R>>
{
public:
  ThunkProcess(std::shared_ptr<lambda::function<R(void)>> _thunk,
               std::shared_ptr<Promise<R>> _promise)
    : thunk(_thunk),
      promise(_promise) {}

  virtual ~ThunkProcess() {}

protected:
  virtual void serve(const Event& event)
  {
    promise->set((*thunk)());
  }

private:
  std::shared_ptr<lambda::function<R(void)>> thunk;
  std::shared_ptr<Promise<R>> promise;
};

} // namespace internal {


template <typename R>
Future<R> run(R (*method)(void))
{
  std::shared_ptr<lambda::function<R(void)>> thunk(
      new lambda::function<R(void)>(
          lambda::bind(method)));

  std::shared_ptr<Promise<R>> promise(new Promise<R>());
  Future<R> future = promise->future();

  terminate(spawn(new internal::ThunkProcess<R>(thunk, promise), true));

  return future;
}


#define TEMPLATE(Z, N, DATA)                                            \
  template <typename R,                                                 \
            ENUM_PARAMS(N, typename P),                                 \
            ENUM_PARAMS(N, typename A)>                                 \
  Future<R> run(                                                        \
      R (*method)(ENUM_PARAMS(N, P)),                                   \
      ENUM_BINARY_PARAMS(N, A, a))                                      \
  {                                                                     \
    std::shared_ptr<lambda::function<R(void)>> thunk(                   \
        new lambda::function<R(void)>(                                  \
            lambda::bind(method, ENUM_PARAMS(N, a))));                  \
                                                                        \
    std::shared_ptr<Promise<R>> promise(new Promise<R>());              \
    Future<R> future = promise->future();                               \
                                                                        \
    terminate(spawn(new internal::ThunkProcess<R>(thunk, promise), true)); \
                                                                        \
    return future;                                                      \
  }

  REPEAT_FROM_TO(1, 11, TEMPLATE, _) // Args A0 -> A9.
#undef TEMPLATE

} // namespace process {

#endif // __PROCESS_RUN_HPP__
