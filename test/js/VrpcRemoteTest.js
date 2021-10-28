'use strict'

/* global describe, it, before, after */

const { assert } = require('chai')
const EventEmitter = require('events')
const { VrpcRemote } = require('vrpc')

const emitter = new EventEmitter()
const newEntries = []
const removedEntries = []

// Register callback for new callbacks
emitter.on('new', entry => newEntries.push(entry))
emitter.on('removed', entry => removedEntries.push(entry))

describe('A javascript VrpcRemote on a C++ Agent', () => {
  let client
  before(async () => {
    client = new VrpcRemote({
      domain: 'test.vrpc',
      token: process.env.VRPC_TEST_TOKEN,
      broker: 'mqtts://vrpc.io:8883'
    })
    await client.connect()
  })

  after(async () => {
    await client.end()
  })

  let testClass
  let anotherTestClass
  it('should be able to create a TestClass proxy using default constructor', async () => {
    testClass = await client.create({ agent: 'cppTestAgent', className: 'TestClass' })
    assert.ok(testClass)
  })
  it('the proxy should have all bound functions as own methods', () => {
    assert.isFunction(testClass.getRegistry)
    assert.isFunction(testClass.hasCategory)
    assert.isFunction(testClass.notifyOnNew)
    assert.isFunction(testClass.notifyOnRemoved)
    assert.isFunction(testClass.addEntry)
    assert.isFunction(testClass.removeEntry)
    assert.isFunction(testClass.callMeBack)
    assert.isNotFunction(testClass.crazy)
  })
  it('getRegistry() should return empty object', async () => {
    assert.isEmpty(await testClass.getRegistry())
  })
  it('hasCategory() should return false for category "test"', async () => {
    assert.isFalse(await testClass.hasCategory('test'))
  })
  it('An EventEmitter instance should be allowed to be registered as callback', async () => {
    await testClass.notifyOnNew({ emitter, event: 'new' })
    await testClass.notifyOnRemoved({ emitter, event: 'removed' })
  })
  it('addEntry() should add a new entry under category "test"', async () => {
    const originalEntry = {
      member1: 'first entry',
      member2: 42,
      member3: 3.14,
      member4: [0, 1, 2, 3]
    }
    await testClass.addEntry('test', originalEntry)
    assert.isTrue(await testClass.hasCategory('test'))
    assert.lengthOf(newEntries, 1)
    assert.lengthOf(removedEntries, 0)
    assert.equal(newEntries[0].member1, originalEntry.member1)
    assert.equal(newEntries[0].member2, originalEntry.member2)
    assert.closeTo(newEntries[0].member3, originalEntry.member3, 0.00001)
    assert.deepEqual(newEntries[0].member4, originalEntry.member4)
    const registry = await testClass.getRegistry()
    assert.equal(registry.test[0].member1, originalEntry.member1)
  })
  it('removeEntry() should remove the entry under category "test"', async () => {
    const entry = await testClass.removeEntry('test')
    assert.equal(entry.member1, 'first entry')
    assert.isFalse(await testClass.hasCategory('test'))
    assert.lengthOf(removedEntries, 1)
  })
  it('should not be possible to remove another entry under category "test"', async () => {
    try {
      await testClass.removeEntry('test')
      assert.isTrue(false)
    } catch (err) {
      assert.equal(err.message, 'Can not remove non-existing category')
    }
  })
  it('should be possible to receive callbacks', async () => {
    await testClass.callMeBack(sleepTime => {
      assert.equal(sleepTime, 100)
    })
  })
  it('should be possible to call a static function', async () => {
    assert.equal(
      await client.callStatic({
        agent: 'cppTestAgent',
        className: 'TestClass',
        functionName: 'crazy'
      }),
      'who is crazy?'
    )
  })
  it('And overloads thereof', async () => {
    assert.equal(
      await client.callStatic({
        agent: 'cppTestAgent',
        className: 'TestClass',
        functionName: 'crazy',
        args: ['vrpc']
      }),
      'vrpc is crazy!'
    )
  })
  it('should not call function with wrong signature', async () => {
    try {
      await client.callStatic({
        agent: 'cppTestAgent',
        className: 'TestClass',
        functionName: 'crazy',
        args: [42]
      })
      assert.isTrue(false)
    } catch (err) {
      assert.equal(err.message, 'Could not find function: crazy-number')
    }
  })
  it('should create a second instance', async () => {
    anotherTestClass = await client.create({
      agent: 'cppTestAgent',
      className: 'TestClass',
      args: [
        {
          testEntry: [{
            member1: 'anotherTestClass',
            member2: 2,
            member3: 2.0,
            member4: [0, 1, 2, 3]
          }]
        }
      ]
    })
    assert.ok(anotherTestClass)
  })
  it('should be well separated from the first instance', async () => {
    const registry = await anotherTestClass.getRegistry()
    console.log('## registry1', registry)
    const registry2 = await testClass.getRegistry()
    console.log('## registry2', registry2)
    // assert.equal(registry.length === 1)
  })
})

describe('Another instance of the VrpcRemote class', () => {
  let vrpc
  it('should be construct-able with pre-defined domain and agent', async () => {
    vrpc = new VrpcRemote({
      domain: 'test.vrpc',
      agent: 'cppTestAgent',
      token: process.env.VRPC_TEST_TOKEN
    })
    await vrpc.connect()
    assert.isObject(vrpc)
  })
  describe('The corresponding VrpcRemote instance', () => {
    let proxy
    it('should be able to create a named proxy instance', async () => {
      proxy = await vrpc.create({
        className: 'TestClass',
        instance: 'test1',
        args: []
      })
      assert.isObject(proxy)
      await proxy.addEntry(
        'test1',
        {
          member1: '1',
          member2: 2,
          member3: 3,
          member4: [4]
        })
    })
    describe('The corresponding TestClass proxy', () => {
      let test1
      it('should have all bound functions as own methods', () => {
        assert.isFunction(proxy.getRegistry)
        assert.isFunction(proxy.hasCategory)
        assert.isFunction(proxy.notifyOnNew)
        assert.isFunction(proxy.notifyOnRemoved)
        assert.isFunction(proxy.addEntry)
        assert.isFunction(proxy.removeEntry)
        assert.isNotFunction(proxy.crazy)
      })
      it('should return the correct registry as provided during construction', async () => {
        const registry = await proxy.getRegistry()
        assert.deepEqual(
          registry,
          { test1: [{ member1: '1', member2: 2, member3: 3, member4: [4] }] }
        )
      })
      it('should not be possible to attach to non-existing instance', async () => {
        try {
          await vrpc.getInstance('bad', { noWait: true })
          assert.fail()
        } catch (err) {
          assert.equal(err.message, 'Could not find instance: bad')
        }
      })
      it('should be possible to attach to the named instance', async () => {
        test1 = await vrpc.getInstance({ className: 'TestClass', instance: 'test1' })
        assert.isObject(test1)
      })
      describe('The attached named instance', () => {
        it('should have all bound functions as own methods', () => {
          assert.isFunction(test1.getRegistry)
          assert.isFunction(test1.hasCategory)
          assert.isFunction(test1.notifyOnNew)
          assert.isFunction(test1.notifyOnRemoved)
          assert.isFunction(test1.addEntry)
          assert.isFunction(test1.removeEntry)
          assert.isNotFunction(test1.crazy)
        })
        it('should return the correct registry', async () => {
          const registry = await test1.getRegistry()
          assert.deepEqual(
            registry,
            { test1: [{ member1: '1', member2: 2, member3: 3, member4: [4] }] }
          )
        })
        it('should be delete-able', async () => {
          const deleted = await vrpc.delete({
            className: 'TestClass',
            instance: 'test1'
          })
          assert.isTrue(deleted)
          try {
            await vrpc.getInstance('test1', { noWait: true })
            assert.fail()
          } catch (err) {
            assert.equal(err.message, 'Could not find instance: test1')
          }
        })
      })
    })
  })
})
