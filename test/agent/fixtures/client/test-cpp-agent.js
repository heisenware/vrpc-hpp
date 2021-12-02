'use strict'

/* global describe, context, before, after, it */
const { VrpcClient } = require('vrpc')
const assert = require('assert')
const sinon = require('sinon')

describe('Testing C++ agent using Node.js client', () => {
  /*******************************
   * agent and class information *
   *******************************/
  describe('(1) agent and class information', () => {
    let client
    const agentSpy = sinon.spy()
    const classSpy = sinon.spy()
    before(async () => {
      // make sure the agents have connected (3 seconds reconnect interval)
      await new Promise(resolve => setTimeout(resolve, 3000))

      client = new VrpcClient({
        broker: 'mqtt://broker',
        domain: 'test.vrpc',
        timeout: 1000
      })
      client.on('agent', agentSpy)
      // client.on('agent', (...args) => console.log('AGENT', args))
      client.on('class', classSpy)
      // client.on('class', (...args) => console.log('CLASS', args))
      await client.connect()
      await new Promise(resolve => setTimeout(resolve, 500))
    })
    after(async () => {
      client.off('agent', agentSpy)
      client.off('class', classSpy)
      await client.end()
    })
    it('should have received agent information after connect', async () => {
      assert(agentSpy.calledTwice)
      assert(
        agentSpy.calledWith({
          domain: 'test.vrpc',
          agent: 'agent1',
          status: 'online',
          hostname: 'agent1',
          version: ''
        })
      )
      assert(
        agentSpy.calledWith({
          domain: 'test.vrpc',
          agent: 'agent2',
          status: 'online',
          hostname: 'agent2',
          version: ''
        })
      )
    })
    it('should have received class information after connect', async () => {
      assert.strictEqual(classSpy.callCount, 3)
      assert(
        classSpy.calledWith({
          domain: 'test.vrpc',
          agent: 'agent1',
          className: 'Foo',
          instances: [],
          memberFunctions: [
            'callback-string:number',
            'reset',
            'increment',
            'onValue-string'
          ],
          staticFunctions: [
            '__createShared__-string:number',
            '__createIsolated__-string:number',
            'staticCallback-string:number',
            'staticIncrement-number',
            '__delete__-string',
            '__createShared__-string',
            '__createIsolated__-string'
          ],
          meta: null
        })
      )
    })
    it.skip('should be possible to unregister the offline agent', async () => {
      const ok = await client.unregisterAgent('agent3')
      assert(ok)
      const agents = Object.keys(client.getSystemInformation())
      assert.strictEqual(agents.length, 2)
      assert(agents.includes('agent1'))
      assert(agents.includes('agent2'))
      const tmpClient = new VrpcClient({
        broker: 'mqtt://broker',
        domain: 'test.vrpc'
      })
      const tmpAgentSpy = sinon.spy()
      tmpClient.on('agent', tmpAgentSpy)
      await tmpClient.connect()
      await new Promise(resolve => setTimeout(resolve, 500))
      assert(tmpAgentSpy.calledTwice)
      tmpClient.off('agent', tmpAgentSpy)
      await tmpClient.end()
    })
    it('should not be possible to unregister an online agent', async () => {
      const ok = await client.unregisterAgent('agent1')
      assert(!ok)
      const agents = Object.keys(client.getSystemInformation())
      assert.strictEqual(agents.length, 2)
    })
    it('should not be possible to unregister a non-existing agent', async () => {
      const ok = await client.unregisterAgent('doesNotExist')
      assert(!ok)
      const agents = Object.keys(client.getSystemInformation())
      assert.strictEqual(agents.length, 2)
    })
  })
  /*******************************
   * proxy creation and deletion *
   *******************************/
  describe('(2) proxy creation and deletion', () => {
    let client
    before(async () => {
      client = new VrpcClient({
        broker: 'mqtt://broker',
        domain: 'test.vrpc',
        timeout: 5000
      })
      await client.connect()
    })
    after(async () => {
      await client.end()
    })
    context('(2.1) when using good options but no instance', () => {
      let proxy1
      let proxy2
      const classSpy = sinon.spy()
      const instanceNewSpy = sinon.spy()
      before(() => {
        client.on('class', classSpy)
        client.on('instanceNew', instanceNewSpy)
      })
      after(() => {
        client.off('class', classSpy)
        client.off('instanceNew', classSpy)
      })
      it('should create an isolated proxy using constructor defaults', async () => {
        proxy1 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          isIsolated: true
        })
        const value = await proxy1.increment()
        assert.strictEqual(value, 1)
      })
      it('should create another isolated proxy using custom arguments', async () => {
        proxy2 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          args: [41],
          isIsolated: true
        })
        const value = await proxy2.increment()
        assert.strictEqual(value, 42)
      })
      it('should not have emitted "class" or "instanceNew" event', () => {
        assert(instanceNewSpy.notCalled)
      })
      it('should not list any available instances', () => {
        const instances = client.getAvailableInstances('Foo', {
          agent: 'agent1'
        })
        assert.strictEqual(instances.length, 0)
      })
      // FIXME Explicit deletion of isolated proxies is not yet implemented
      it.skip('should delete the isolated instances', async () => {
        const result1 = await client.delete(proxy1)
        const result2 = await client.delete(proxy2)
        assert.strictEqual(result1, true)
        assert.strictEqual(result2, true)
      })
    })
    context('(2.2) when using good options and instance', () => {
      let proxy1
      let proxy2
      const classSpy = sinon.spy()
      const instanceNewSpy = sinon.spy()
      const instanceGoneSpy = sinon.spy()
      before(() => {
        client.on('class', classSpy)
        // client.on('class', (...args) => console.log('CLASS', args))
        client.on('instanceNew', instanceNewSpy)
        client.on('instanceGone', instanceGoneSpy)
      })
      after(() => {
        client.off('class', classSpy)
        client.off('instanceNew', instanceNewSpy)
        client.off('instanceGone', instanceGoneSpy)
      })
      it('should create a shared proxy using constructor defaults', async () => {
        proxy1 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          instance: 'instance1'
        })
        const value = await proxy1.increment()
        assert.strictEqual(value, 1)
      })
      it('should create another shared proxy using custom arguments', async () => {
        proxy2 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          instance: 'instance2',
          args: [41]
        })
        const value = await proxy2.increment()
        assert.strictEqual(value, 42)
      })
      it('should have emitted "class" and "instanceNew" event', () => {
        assert(classSpy.calledTwice)
        assert.deepStrictEqual(classSpy.args[0][0], {
          domain: 'test.vrpc',
          agent: 'agent1',
          className: 'Foo',
          instances: ['instance1'],
          memberFunctions: [
            'callback-string:number',
            'reset',
            'increment',
            'onValue-string'
          ],
          staticFunctions: [
            '__createShared__-string:number',
            '__createIsolated__-string:number',
            'staticCallback-string:number',
            'staticIncrement-number',
            '__delete__-string',
            '__createShared__-string',
            '__createIsolated__-string'
          ],
          meta: null
        })
        assert(instanceNewSpy.calledTwice)
        assert.deepStrictEqual(instanceNewSpy.args[1][0], ['instance2'])
        assert.deepStrictEqual(instanceNewSpy.args[1][1], {
          domain: 'test.vrpc',
          agent: 'agent1',
          className: 'Foo'
        })
      })
      it('should list the available instances', () => {
        const instances = client.getAvailableInstances('Foo', {
          agent: 'agent1'
        })
        assert.deepStrictEqual(instances, ['instance2', 'instance1'])
      })
      it('should delete the shared instances', async () => {
        const result1 = await client.delete('instance1')
        assert.strictEqual(result1, true)
        assert.strictEqual(classSpy.callCount, 3)
        assert.strictEqual(classSpy.args[2][0].instances.length, 1)
        assert.strictEqual(instanceGoneSpy.callCount, 1)
        const result2 = await client.delete('instance2')
        assert.strictEqual(result2, true)
        assert.strictEqual(classSpy.callCount, 4)
        assert.strictEqual(classSpy.args[3][0].instances.length, 0)
        assert.strictEqual(instanceGoneSpy.callCount, 2)
      })
      it.skip('should contain proper instance-, and client ids', () => {
        assert.strictEqual(proxy1.vrpcClientId, client.getClientId())
        assert.strictEqual(proxy1.vrpcInstanceId, 'instance1')
        assert.strictEqual(proxy2.vrpcClientId, client.getClientId())
        assert.strictEqual(proxy2.vrpcInstanceId, 'instance2')
        assert(proxy1.vrpcProxyId !== proxy2.vrpcProxyId)
      })
    })
  })
  /*************************
   * remote function calls *
   *************************/
  describe('(3) remote function calls', () => {
    let client
    before(async () => {
      client = new VrpcClient({
        broker: 'mqtt://broker',
        domain: 'test.vrpc'
      })
      await client.connect()
    })
    after(async () => {
      await client.end()
    })
    context('(3.1) static context', () => {
      it('should work for synchronous functions with arguments', async () => {
        const value = await client.callStatic({
          agent: 'agent2',
          className: 'Bar',
          functionName: 'staticIncrement',
          args: [41]
        })
        assert.strictEqual(value, 42)
      })
      it('should work for functions with callback arguments', async () => {
        const callbackSpy = sinon.spy()
        await client.callStatic({
          agent: 'agent1',
          className: 'Foo',
          functionName: 'staticCallback',
          args: [callbackSpy, 50]
        })
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledOnce)
        assert.strictEqual(callbackSpy.args[0][0], 50)
      })
    })
    context('(3.2) instance context', () => {
      let agent1Foo1
      let agent1Foo2
      let agent2Foo1
      before(async () => {
        agent1Foo1 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          instance: 'agent1Foo1'
        })
        agent1Foo2 = await client.create({
          agent: 'agent1',
          className: 'Foo',
          instance: 'agent1Foo2',
          args: [1]
        })
        agent2Foo1 = await client.create({
          agent: 'agent2',
          className: 'Foo',
          instance: 'agent2Foo1',
          args: [2]
        })
        await client.create({
          agent: 'agent2',
          className: 'Foo',
          instance: 'agent2Foo2',
          args: [2]
        })
      })
      after(async () => {
        await client.delete('agent1Foo1')
      })
      it('should work for synchronous functions', async () => {
        const value = await agent1Foo1.increment()
        assert.strictEqual(value, 1)
      })
      it('should work for functions with callback arguments', async () => {
        const callbackSpy = sinon.spy()
        await agent1Foo1.callback(callbackSpy, 50)
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledOnce)
        assert.strictEqual(callbackSpy.args[0][0], 1)
      })
      it.skip('should allow batch-calling synchronous functions on a single agent', async () => {
        const value = await client.callAll({
          agent: 'agent1',
          className: 'Foo',
          functionName: 'increment'
        })
        assert.deepStrictEqual(
          value.map(({ val }) => val),
          [2, 2]
        )
        assert.deepStrictEqual(
          value.map(({ err }) => err),
          [null, null]
        )
        const ids = value.map(({ id }) => id)
        assert(ids.includes('agent1Foo1'))
        assert(ids.includes('agent1Foo2'))
      })
      it.skip('should allow batch-calling asynchronous functions on a single agent', async () => {
        const value = await client.callAll({
          agent: 'agent2',
          className: 'Foo',
          functionName: 'resolvePromise'
        })
        assert.deepStrictEqual(
          value.map(({ val }) => val),
          [2, 2]
        )
        assert.deepStrictEqual(
          value.map(({ err }) => err),
          [null, null]
        )
        const ids = value.map(({ id }) => id)
        assert(ids.includes('agent2Foo1'))
        assert(ids.includes('agent2Foo2'))
      })
      it.skip('should allow batch-calling synchronous functions across agents', async () => {
        const value = await client.callAll({
          className: 'Foo',
          functionName: 'increment'
        })
        assert.deepStrictEqual(
          value.map(({ val }) => val),
          [3, 3, 3, 3]
        )
        assert.deepStrictEqual(
          value.map(({ err }) => err),
          [null, null, null, null]
        )
        const ids = value.map(({ id }) => id)
        assert(ids.includes('agent1Foo1'))
        assert(ids.includes('agent1Foo2'))
        assert(ids.includes('agent2Foo1'))
        assert(ids.includes('agent2Foo2'))
      })
      it.skip('should allow batch-calling asynchronous functions across agents', async () => {
        const value = await client.callAll({
          agent: '*',
          className: 'Foo',
          functionName: 'resolvePromise'
        })
        assert.deepStrictEqual(
          value.map(({ val }) => val),
          [3, 3, 3, 3]
        )
        assert.deepStrictEqual(
          value.map(({ err }) => err),
          [null, null, null, null]
        )
        const ids = value.map(({ id }) => id)
        assert(ids.includes('agent1Foo1'))
        assert(ids.includes('agent1Foo2'))
        assert(ids.includes('agent2Foo1'))
        assert(ids.includes('agent2Foo2'))
      })
      it.skip('should allow batch event-registration on a single agent', async () => {
        const callbackSpy = sinon.spy()
        await client.callAll({
          agent: 'agent1',
          className: 'Foo',
          functionName: 'on',
          args: ['value', callbackSpy]
        })
        await agent1Foo1.increment()
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledOnce)
        assert.strictEqual(callbackSpy.args[0][0], 'agent1Foo1')
        assert.strictEqual(callbackSpy.args[0][1], 4)
        await agent1Foo2.increment()
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledTwice)
        assert.strictEqual(callbackSpy.args[1][0], 'agent1Foo2')
        assert.strictEqual(callbackSpy.args[1][1], 4)
      })
      it.skip('should allow batch event-registration across agents', async () => {
        const callbackSpy = sinon.spy()
        await client.callAll({
          agent: '*',
          className: 'Foo',
          functionName: 'on',
          args: ['value', callbackSpy]
        })
        await agent1Foo1.increment()
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledOnce)
        assert.strictEqual(callbackSpy.args[0][0], 'agent1Foo1')
        assert.strictEqual(callbackSpy.args[0][1], 5)
        await agent2Foo1.increment()
        await new Promise(resolve => setTimeout(resolve, 100))
        assert(callbackSpy.calledTwice)
        assert.strictEqual(callbackSpy.args[1][0], 'agent2Foo1')
        assert.strictEqual(callbackSpy.args[1][1], 4)
      })
    })
  })
})
